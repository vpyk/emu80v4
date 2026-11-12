// McpServer.cpp: embedded MCP server for Emu80.
//
// HTTP listener on 127.0.0.1, JSON-RPC 2.0 + MCP protocol "2024-11-05".
// All emulator access happens on the main thread via mcp::Run().

// Third-party headers first (httplib needs winsock2 before windows.h on MSVC).
#include "3rdparty/httplib.h"
#include "3rdparty/json.hpp"

// stb_image_write for PNG screen capture.
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "3rdparty/stb_image_write.h"

// Emu80 headers.
#include "../Globals.h"
#include "../Emulation.h"
#include "../EmuCalls.h"
#include "../Platform.h"
#include "../DebugSession.h"
#include "../Cpu.h"
#include "../DebugTarget.h"
#include "../CrtRenderer.h"
#include "../AddrSpace.h"
#include "../EmuConfig.h"
#include "../Keyboard.h"
#include "../KbdLayout.h"
#include "../KbdTapper.h"
#include "../MouseDevice.h"
#include "../Pal.h"

#include "McpServer.h"
#include "McpMarshal.h"

#include <functional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdio>

using json = nlohmann::json;

namespace
{
    // -----------------------------------------------------------------------
    // Constants
    // -----------------------------------------------------------------------
    constexpr char  MCP_PROTOCOL_VERSION[] = "2024-11-05";
    constexpr char  MCP_SERVER_NAME[]      = "Emu80";
    constexpr char  MCP_SERVER_VERSION[]   = VER_STR;

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    std::string Hex(uint16_t v)
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%04X", static_cast<unsigned>(v));
        return std::string(buf);
    }

    // At least 4 digits, more for addresses above 64K
    std::string Hex32(uint32_t v)
    {
        char buf[12];
        std::snprintf(buf, sizeof(buf), "%04X", static_cast<unsigned>(v));
        return std::string(buf);
    }

    std::string Oct(uint16_t v)
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%06o", static_cast<unsigned>(v));
        return std::string(buf);
    }

    json RegVal(uint16_t v)
    {
        return json{ { "hex", Hex(v) }, { "dec", static_cast<int>(v) } };
    }

    // Returns the current platform.
    // Safe to call from any thread for null check; dereference on main thread only.
    Platform* GetPlatform()
    {
        if (!g_emulation) return nullptr;
        Platform* p = g_emulation->getCurrentPlatform();
        return p;
    }

    // Parses a numeric argument.
    // JSON integer → always decimal (JSON spec).
    // JSON string → hex by default. Prefixes: 0x/0X = hex, 0o/0O = octal,
    // a lone leading zero (no x/o) = decimal.
    // Parses a number argument (32 bits)
    uint32_t ParseNumArg32(const json& v, const char* what)
    {
        try {
            if (v.is_number_integer())
                return static_cast<uint32_t>(v.get<long long>());

            if (v.is_string()) {
                std::string s = v.get<std::string>();
                const size_t b = s.find_first_not_of(" \t");
                const size_t e = s.find_last_not_of(" \t");
                if (b != std::string::npos)
                    s = s.substr(b, e - b + 1);

                int base = 16;
                if (s.size() >= 2 && s[0] == '0') {
                    if (s[1] == 'x' || s[1] == 'X')
                        base = 16;
                    else if (s[1] == 'o' || s[1] == 'O')
                        base = 8;
                    else
                        base = 10;
                }

                size_t pos = 0;
                const unsigned long val = std::stoul(s, &pos, base);
                if (pos == s.size())
                    return static_cast<uint32_t>(val);
            }
        } catch (...) {
        }

        throw std::runtime_error(std::string("invalid ") + what + " value");
    }

    uint16_t ParseNumArg(const json& v, const char* what)
    {
        return static_cast<uint16_t>(ParseNumArg32(v, what) & 0xFFFF);
    }

    bool OptBool(const json& args, const char* key, bool def)
    {
        if (!args.contains(key) || args[key].is_null())
            return def;
        if (!args[key].is_boolean())
            throw std::runtime_error(std::string("invalid ") + key + " (expected boolean)");
        return args[key].get<bool>();
    }

    std::string Base64(const uint8_t* data, size_t len)
    {
        static const char tbl[] =
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string out;
        out.reserve(((len + 2) / 3) * 4);
        size_t i = 0;
        for (; i + 3 <= len; i += 3) {
            const uint32_t n = (static_cast<uint32_t>(data[i]) << 16) |
                               (static_cast<uint32_t>(data[i + 1]) << 8) |
                               static_cast<uint32_t>(data[i + 2]);
            out.push_back(tbl[(n >> 18) & 63]);
            out.push_back(tbl[(n >> 12) & 63]);
            out.push_back(tbl[(n >> 6) & 63]);
            out.push_back(tbl[n & 63]);
        }
        if (i < len) {
            const bool     two = (i + 1 < len);
            const uint32_t n   = (static_cast<uint32_t>(data[i]) << 16) |
                                 (two ? (static_cast<uint32_t>(data[i + 1]) << 8) : 0);
            out.push_back(tbl[(n >> 18) & 63]);
            out.push_back(tbl[(n >> 12) & 63]);
            out.push_back(two ? tbl[(n >> 6) & 63] : '=');
            out.push_back('=');
        }
        return out;
    }

    // -----------------------------------------------------------------------
    // Key name → EmuKey lookup
    // -----------------------------------------------------------------------
    EmuKey ParseKeyName(const std::string& name)
    {
        static const std::map<std::string, EmuKey> keyMap = {
            {"0",EK_0},{"1",EK_1},{"2",EK_2},{"3",EK_3},{"4",EK_4},
            {"5",EK_5},{"6",EK_6},{"7",EK_7},{"8",EK_8},{"9",EK_9},
            {"a",EK_A},{"b",EK_B},{"c",EK_C},{"d",EK_D},{"e",EK_E},
            {"f",EK_F},{"g",EK_G},{"h",EK_H},{"i",EK_I},{"j",EK_J},
            {"k",EK_K},{"l",EK_L},{"m",EK_M},{"n",EK_N},{"o",EK_O},
            {"p",EK_P},{"q",EK_Q},{"r",EK_R},{"s",EK_S},{"t",EK_T},
            {"u",EK_U},{"v",EK_V},{"w",EK_W},{"x",EK_X},{"y",EK_Y},
            {"z",EK_Z},
            {"space",EK_SPACE},{"enter",EK_CR},{"esc",EK_ESC},
            {"tab",EK_TAB},{"backspace",EK_BSP},{"caps",EK_LANG},
            {"shift",EK_SHIFT},{"ctrl",EK_CTRL},
            {"f1",EK_F1},{"f2",EK_F2},{"f3",EK_F3},{"f4",EK_F4},
            {"f5",EK_F5},{"f6",EK_F6},{"f7",EK_F7},{"f8",EK_F8},
            {"f9",EK_F9},{"f10",EK_F10},{"f11",EK_F11},
            {"up",EK_UP},{"down",EK_DOWN},{"left",EK_LEFT},{"right",EK_RIGHT},
            {"home",EK_HOME},{"end",EK_END},
            {"ins",EK_INS},{"del",EK_DEL},
            {"minus",EK_MINUS},{"equals",EK_EQU},
            {"lbracket",EK_LBRACKET},{"rbracket",EK_RBRACKET},
            {"backslash",EK_BKSLASH},{"semicolon",EK_SEMICOLON},
            {"comma",EK_COMMA},{"period",EK_PERIOD},{"slash",EK_SLASH},
            {"grave",EK_GRAVE},{"colon",EK_COLON},
            {"caret",EK_CARET},{"at",EK_AT},{"tilde",EK_TILDE},
            {"lbrace",EK_LBRACE},{"rbrace",EK_RBRACE},
            {"clear",EK_CLEAR},
            {"stop",EK_STOP},{"exec",EK_EXEC},{"reset",EK_RESET},
            {"help",EK_HELP},{"lf",EK_LF},{"rpt",EK_RPT},
            {"np0",EK_NP_0},{"np1",EK_NP_1},{"np2",EK_NP_2},
            {"np3",EK_NP_3},{"np4",EK_NP_4},{"np5",EK_NP_5},
            {"np6",EK_NP_6},{"np7",EK_NP_7},{"np8",EK_NP_8},
            {"np9",EK_NP_9},{"np_period",EK_NP_PERIOD},
            {"np_comma",EK_NP_COMMA},{"np_enter",EK_NP_CR},
            {"js_up",EK_JS_UP},{"js_down",EK_JS_DOWN},
            {"js_left",EK_JS_LEFT},{"js_right",EK_JS_RIGHT},
            {"js_btn1",EK_JS_BTN1},{"js_btn2",EK_JS_BTN2},
        };
        auto it = keyMap.find(name);
        if (it != keyMap.end())
            return it->second;
        throw std::runtime_error("unknown key: " + name);
    }

    // -----------------------------------------------------------------------
    // Tool registry
    // -----------------------------------------------------------------------

    struct McpTool
    {
        std::string                          name;
        std::string                          description;
        json                                 inputSchema;
        std::function<json(const json& args)> handler;
    };

    // ================================================================
    // Helpers for the CPU debug target. Must be called on the main thread.
    // ================================================================

    // Debugger view of the platform CPU, nullptr if the CPU is not supported
    CpuDebugTarget* GetTarget(Platform* p)
    {
        DebugSession* session = p ? p->getDebugSession() : nullptr;
        return session ? session->getTarget() : nullptr;
    }

    // Why the emulation is stopped: stop_reason ("break", "breakpoint", "step",
    // "timer", "halt", "illegal") and breakpoint_id for a breakpoint stop
    void AddStopInfo(Platform* p, json& result)
    {
        DebugSession* session = p ? p->getDebugSession() : nullptr;
        if (!session || session->isRunning() || !g_emulation->isDebuggerActive())
            return;
        const DebugStopInfo& info = session->getStopInfo();
        result["stop_reason"] = getStopReasonName(info.reason);
        if (info.reason == DSR_BREAKPOINT)
            result["breakpoint_id"] = info.bpId;
    }

    std::string InstrText(CpuDebugTarget* cpu, uint16_t addr)
    {
        DisasmInstr instr;
        cpu->disassemble(addr, instr);
        return instr.text;
    }

    // Register value by id (0 if the CPU has no such register)
    unsigned RegById(CpuDebugTarget* cpu, const char* id)
    {
        int index = cpu->findReg(id);
        return index >= 0 ? cpu->getReg(index) : 0;
    }

    // All registers except single flags, {id: {hex, dec}}
    json AllRegsJson(CpuDebugTarget* cpu)
    {
        json regs = json::object();
        for (int i = 0; i < cpu->getRegCount(); i++) {
            const DebugRegInfo& info = cpu->getRegInfo(i);
            if (info.group != DRG_FLAG)
                regs[info.id] = RegVal(static_cast<uint16_t>(cpu->getReg(i)));
        }
        return regs;
    }

    // ================================================================
    // Helper: fill json with CPU state (registers, flags, Z80 alt regs,
    // instruction at PC). Must be called on the main thread.
    // ================================================================
    void FillCpuStateJson(CpuDebugTarget* cpu, json& state)
    {
        std::string cpuType = cpu->getCpuName();
        state["cpu_type"] = cpuType;

        // main registers, PC and SP
        json regs = json::object();
        for (int i = 0; i < cpu->getRegCount(); i++) {
            const DebugRegInfo& info = cpu->getRegInfo(i);
            if (info.group == DRG_MAIN && !(cpuType == "z80" && (std::string(info.id) == "ix" || std::string(info.id) == "iy")))
                regs[info.id] = RegVal(static_cast<uint16_t>(cpu->getReg(i)));
        }
        state["registers"] = regs;

        if (cpu->findReg("af") >= 0) {
            const uint16_t f = RegById(cpu, "af") & 0xFF;
            state["psw"] = RegVal(f);
            state["flags"] = {
                { "S", !!(f & 0x80) }, { "Z", !!(f & 0x40) },
                { "H", !!(f & 0x10) }, { "P", !!(f & 0x04) },
                { "N", !!(f & 0x02) }, { "C", !!(f & 0x01) },
            };
        }
        state["iff"] = RegById(cpu, "iff") != 0;

        if (cpuType == "z80") {
            json alt;
            for (const char* id : { "af2", "bc2", "de2", "hl2", "ix", "iy", "i", "r" })
                alt[id] = RegVal(static_cast<uint16_t>(RegById(cpu, id)));
            alt["im"] = RegById(cpu, "im");
            state["z80_regs"] = alt;
        }

        // Current instruction at PC
        state["instruction"] = InstrText(cpu, cpu->getPC());
    }

    // ================================================================
    // Tool: cpu_state
    // ================================================================
    json Tool_CpuState(const json& /*args*/)
    {
        json        state;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }

            state["breaked"]   = g_emulation->isDebuggerActive();
            state["paused"]    = g_emulation->getPausedState();
            state["cpu_ticks"] = p->getCpuElapsedTicks();
            AddStopInfo(p, state);
            FillCpuStateJson(cpu, state);
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return state;
    }

    // ================================================================
    // Tool: regs_set
    // ================================================================
    json Tool_RegsSet(const json& args)
    {
        // Parse all keys upfront; the names are checked against the CPU on the main thread
        std::vector<std::string>  names;
        std::vector<uint16_t>     values;
        for (auto it = args.begin(); it != args.end(); ++it) {
            std::string key = it.key();
            for (char& c : key)
                if (c >= 'A' && c <= 'Z')
                    c = static_cast<char>(c - 'A' + 'a');
            names.push_back(key);
            values.push_back(ParseNumArg(it.value(), key.c_str()));
        }

        if (names.empty())
            throw std::runtime_error("specify at least one register");

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }

            if (!g_emulation->isDebuggerActive()) {
                err = "CPU is not paused; call debug_break first";
                return;
            }

            // check all names first, so that nothing is changed on an error
            std::vector<int> indexes;
            for (const std::string& n : names) {
                int index = cpu->findReg(n == "psw" ? "af" : n);
                if (index < 0) {
                    std::string valid;
                    for (int i = 0; i < cpu->getRegCount(); i++)
                        valid += std::string(valid.empty() ? "" : ", ") + cpu->getRegInfo(i).id;
                    if (cpu->findReg("af") >= 0)
                        valid += ", psw";
                    err = "unknown register '" + n + "' (" + cpu->getCpuName() + ": " + valid + ")";
                    return;
                }
                indexes.push_back(index);
            }

            DebugSession* session = p->getDebugSession();
            for (size_t i = 0; i < names.size(); ++i) {
                uint16_t v = values[i];
                if (names[i] == "psw") // low byte of AF
                    v = (cpu->getReg(indexes[i]) & 0xFF00) | (v & 0xFF);
                session->setReg(indexes[i], v);
            }

            // Read back
            result["registers"] = AllRegsJson(cpu);
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: sys_info
    // ================================================================
    json Tool_SysInfo(const json& /*args*/)
    {
        json info;
        info["server"] = {
            { "name",     MCP_SERVER_NAME },
            { "version",  MCP_SERVER_VERSION },
            { "protocol", MCP_PROTOCOL_VERSION },
            { "port",     g_McpServer.GetPort() },
        };

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            info["platform"] = p ? p->getBaseName() : "(none)";

            if (p) {
                Cpu* cpu = p->getCpu();
                CpuDebugTarget* target = GetTarget(p);
                json& cpuInfo = info["cpu"];
                cpuInfo["type"] = target ? target->getCpuName() : "unknown";
                cpuInfo["frequency_hz"] = cpu ? g_emulation->getFrequency() / cpu->getKDiv() : 0;
                cpuInfo["breaked"] = g_emulation->isDebuggerActive();
                cpuInfo["paused"]  = g_emulation->getPausedState();
                cpuInfo["speed_factor"] = g_emulation->getSpeedUpFactor();
                cpuInfo["full_throttle"] = g_emulation->getFullThrottleState();
            }

            // Available platforms
            EmuConfig* ec = g_emulation->getConfig();
            if (ec) {
                json ap = json::array();
                for (const auto& pi : *ec->getPlatformInfos())
                    ap.push_back({ { "name", pi.platformName }, { "platform", pi.objName } });
                info["available_platforms"] = ap;
            }
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        return info;
    }

    // ================================================================
    // Tool: disasm
    // ================================================================
    constexpr int DISASM_DEFAULT_COUNT = 16;
    constexpr int DISASM_MAX_COUNT     = 64;

    json Tool_Disasm(const json& args)
    {
        const bool hasAddr = args.contains("addr") && !args["addr"].is_null();
        const uint16_t reqAddr = hasAddr ? ParseNumArg(args["addr"], "addr") : 0;
        int count = DISASM_DEFAULT_COUNT;
        if (args.contains("count") && args["count"].is_number_integer()) {
            count = args["count"].get<int>();
            if (count < 1) count = 1;
            if (count > DISASM_MAX_COUNT) count = DISASM_MAX_COUNT;
        }

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }
            uint16_t pc = hasAddr ? reqAddr : cpu->getPC();
            result["start"] = Hex(pc);

            json items = json::array();
            for (int n = 0; n < count; ++n) {
                DisasmInstr instr;
                cpu->disassemble(pc, instr);
                unsigned len = instr.length;

                json item;
                item["addr"] = Hex(pc);
                item["text"]     = instr.text;
                item["len_bytes"] = static_cast<int>(len);

                json bytesArr = json::array();
                for (unsigned i = 0; i < len && i < 4; ++i)
                    bytesArr.push_back(Hex(cpu->readMem(static_cast<uint16_t>(pc + i))));
                item["bytes"] = bytesArr;

                items.push_back(item);
                pc = static_cast<uint16_t>(pc + len);
            }
            result["instructions"] = items;
            result["next"] = Hex(pc);
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: mem_read
    // ================================================================
    constexpr int MEM_READ_DEFAULT_LEN = 64;
    constexpr int MEM_READ_MAX_LEN     = 4096;

    // Optional 'device' argument of the memory tools: a memory name from mem_devices;
    // the CPU address space if omitted
    std::string DeviceArg(const json& args)
    {
        if (!args.contains("device") || args["device"].is_null())
            return "cpu";
        if (!args["device"].is_string())
            throw std::runtime_error("device must be a string (see mem_devices)");
        return args["device"].get<std::string>();
    }

    // Finds the memory on the main thread; sets err if not found
    int FindMemory(DebugSession* session, const std::string& device, std::string& err)
    {
        int mem = session->findMemory(device);
        if (mem < 0)
            err = "unknown device '" + device + "' (see mem_devices)";
        return mem;
    }

    json Tool_MemRead(const json& args)
    {
        if (!args.contains("addr") || args["addr"].is_null())
            throw std::runtime_error("addr is required");

        const uint32_t addr32 = ParseNumArg32(args["addr"], "addr");
        const std::string device = DeviceArg(args);
        int len = MEM_READ_DEFAULT_LEN;
        if (args.contains("len") && !args["len"].is_null()) {
            const json& L = args["len"];
            if (L.is_number_integer())
                len = L.get<int>();
            else if (L.is_string())
                len = std::stoi(L.get<std::string>());
            else
                throw std::runtime_error("invalid len value");
        }
        if (len < 1) len = 1;
        if (len > MEM_READ_MAX_LEN) len = MEM_READ_MAX_LEN;

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            DebugSession* session = p->getDebugSession();
            if (!session) { err = "CPU is not available"; return; }
            int mem = FindMemory(session, device, err);
            if (mem < 0) return;
            const unsigned size = session->getMemories()[mem].size;

            // the CPU address space wraps around, other memories end at their size
            uint32_t addr = mem == 0 ? (addr32 & 0xFFFF) : addr32;
            if (addr >= size) { err = "addr is out of the device (size " + Hex32(size) + ")"; return; }
            if (mem != 0 && addr + len > size)
                len = static_cast<int>(size - addr);

            static const char hexd[] = "0123456789abcdef";
            std::string hex, ascii;
            hex.reserve(static_cast<size_t>(len) * 2);
            ascii.reserve(static_cast<size_t>(len));

            for (int i = 0; i < len; ++i) {
                const uint8_t b = session->readMem(mem, (addr + i) % size);
                hex.push_back(hexd[b >> 4]);
                hex.push_back(hexd[b & 0x0F]);
                ascii.push_back((b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.');
            }

            if (mem != 0)
                result["device"] = device;
            result["addr"]  = Hex32(addr);
            result["len"]   = len;
            result["end"]   = Hex32(mem == 0 ? ((addr + len) & 0xFFFF) : addr + len);
            result["hex"]   = hex;
            result["ascii"] = ascii;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: mem_write
    // ================================================================
    constexpr int MEM_WRITE_MAX_LEN = 4096;

    json Tool_MemWrite(const json& args)
    {
        if (!args.contains("addr") || args["addr"].is_null())
            throw std::runtime_error("addr is required");
        if (!args.contains("data") || !args["data"].is_string())
            throw std::runtime_error("data (hex string) is required");

        const uint32_t addr32 = ParseNumArg32(args["addr"], "addr");
        const std::string device = DeviceArg(args);
        std::string hexStr = args["data"].get<std::string>();

        // Strip non-hex characters.
        std::string clean;
        for (char c : hexStr)
            if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))
                clean.push_back(c);
        if (clean.empty())
            throw std::runtime_error("data contains no hex digits");
        if (clean.size() % 2 != 0)
            throw std::runtime_error("data must have even number of hex digits");
        if (static_cast<int>(clean.size() / 2) > MEM_WRITE_MAX_LEN)
            throw std::runtime_error("data too long (max 4096 bytes)");

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            DebugSession* session = p->getDebugSession();
            if (!session) { err = "CPU is not available"; return; }
            int mem = FindMemory(session, device, err);
            if (mem < 0) return;
            const unsigned size = session->getMemories()[mem].size;

            // the CPU address space wraps around, other memories end at their size
            uint32_t addr = mem == 0 ? (addr32 & 0xFFFF) : addr32;
            const unsigned n = static_cast<unsigned>(clean.size() / 2);
            if (addr >= size || (mem != 0 && addr + n > size)) {
                err = "data is out of the device (size " + Hex32(size) + ")";
                return;
            }

            int count = 0;
            for (size_t i = 0; i < clean.size(); i += 2) {
                uint8_t byteVal = static_cast<uint8_t>(
                    std::stoul(clean.substr(i, 2), nullptr, 16));
                session->writeMem(mem, (addr + count) % size, byteVal);
                ++count;
            }

            if (mem != 0)
                result["device"] = device;
            result["addr"]  = Hex32(addr);
            result["count"] = count;
            result["end"]   = Hex32(mem == 0 ? ((addr + count) & 0xFFFF) : addr + count);
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: port_read
    // ================================================================
    json Tool_PortRead(const json& args)
    {
        if (!args.contains("port") || args["port"].is_null())
            throw std::runtime_error("port is required");

        const uint16_t port = ParseNumArg(args["port"], "port") & 0xFF;

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            Cpu* cpu = p->getCpu();
            if (!cpu) { err = "CPU is not available"; return; }
            AddressableDevice* io = cpu->getIoAddrSpace();
            if (!io) { err = "no I/O address space"; return; }

            uint8_t value = io->readByte(port);
            result["port"]  = Hex(static_cast<uint16_t>(port));
            result["value"] = Hex(static_cast<uint16_t>(value));
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: port_write
    // ================================================================
    json Tool_PortWrite(const json& args)
    {
        if (!args.contains("port") || args["port"].is_null())
            throw std::runtime_error("port is required");
        if (!args.contains("value") || args["value"].is_null())
            throw std::runtime_error("value is required");

        const uint16_t port  = ParseNumArg(args["port"],  "port") & 0xFF;
        const uint16_t value = ParseNumArg(args["value"], "value") & 0xFF;

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            Cpu* cpu = p->getCpu();
            if (!cpu) { err = "CPU is not available"; return; }
            AddressableDevice* io = cpu->getIoAddrSpace();
            if (!io) { err = "no I/O address space"; return; }

            io->writeByte(port, static_cast<uint8_t>(value));
            if (DebugSession* session = p->getDebugSession())
                session->notifyModified();
            result["port"]  = Hex(static_cast<uint16_t>(port));
            result["value"] = Hex(value);
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: screen
    // ================================================================
    json Tool_Screen(const json& /*args*/)
    {
        std::string b64;
        json        meta;
        std::string err;
        std::vector<uint8_t> pngBuf;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CrtRenderer* renderer = p->getRenderer();
            if (!renderer) { err = "no renderer"; return; }

            // Force a render cycle even in paused/debug mode.
            renderer->prepareDebugScreen();

            EmuPixelData pd = renderer->getPixelData();
            if (!pd.pixelData) { err = "no pixel data available"; return; }

            int w = pd.width;
            int h = pd.height;

            // Emu80's pixelData is uint32_t ARGB (0xAARRGGBB).
            // On little-endian: byte[0]=B, byte[1]=G, byte[2]=R, byte[3]=A.
            // Convert to RGBA (stb expects R,G,B,A byte order), force alpha=0xFF.
            std::vector<uint32_t> rgba(w * h);
            for (int i = 0; i < w * h; ++i) {
                uint32_t px = pd.pixelData[i];
                uint8_t b = static_cast<uint8_t>(px & 0xFF);
                uint8_t g = static_cast<uint8_t>((px >> 8) & 0xFF);
                uint8_t r = static_cast<uint8_t>((px >> 16) & 0xFF);
                rgba[i] = (r << 0) | (g << 8) | (b << 16) | (0xFF << 24);
            }

            // Encode to PNG in memory via callback.
            pngBuf.clear();
            auto writeCb = [](void* ctx, void* data, int sz) {
                auto* vec = static_cast<std::vector<uint8_t>*>(ctx);
                auto* ptr = static_cast<uint8_t*>(data);
                vec->insert(vec->end(), ptr, ptr + sz);
            };

            if (!stbi_write_png_to_func(writeCb, &pngBuf, w, h, 4, rgba.data(), w * 4)) {
                err = "failed to encode PNG";
                return;
            }

            b64 = Base64(pngBuf.data(), pngBuf.size());
            meta = {
                { "width",  w },
                { "height", h },
                { "format", "png" },
                { "bytes",  static_cast<int>(pngBuf.size()) },
            };
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);

        // Rich result: image + text metadata.
        json result = json{
            { "_content", json::array({
                json{ { "type", "image" }, { "data", b64 }, { "mimeType", "image/png" } },
                json{ { "type", "text" }, { "text", meta.dump(2) } },
            })}
        };

        return result;
    }

    // ================================================================
    // Tool: text_screen
    // ================================================================
    json Tool_TextScreen(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CrtRenderer* renderer = p->getRenderer();
            if (!renderer) { err = "no renderer"; return; }

            const char* text = renderer->getTextScreen();
            if (!text) {
                err = "text screen not available on this platform";
                return;
            }
            result["text"] = text;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: emu_run
    // ================================================================
    json Tool_EmuRun(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }

            // The session lets execution leave a breakpoint at the current PC
            DebugSession* session = p->getDebugSession();
            if (!session) { err = "debugger not available in MCP mode"; return; }

            const bool wasBreaked = g_emulation->isDebuggerActive();
            session->run(DC_EXTERNAL);

            result["breaked"]  = g_emulation->isDebuggerActive();
            result["changed"]  = wasBreaked;
            result["pc"]       = Hex(cpu->getPC());
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: emu_run_for
    // ================================================================
    constexpr int RUN_FOR_MAX_MS = 60000;

    json Tool_EmuRunFor(const json& args)
    {
        if (!args.contains("ms") || !args["ms"].is_number_integer())
            throw std::runtime_error("ms (integer, milliseconds) is required");
        int ms = args["ms"].get<int>();
        if (ms < 1 || ms > RUN_FOR_MAX_MS)
            throw std::runtime_error("ms must be between 1 and " + std::to_string(RUN_FOR_MAX_MS));

        json        result;
        std::string err;

        // Phase 1: set up timer and start run.
        const bool ok1 = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }

            DebugSession* session = p->getDebugSession();
            if (!session) { err = "debugger not available in MCP mode"; return; }

            session->runFor(static_cast<unsigned>(ms), DC_EXTERNAL);
            result["ms"] = ms;
        });

        if (!ok1) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);

        // Phase 2: poll until CPU stops (use real elapsed time).
        auto start = std::chrono::steady_clock::now();
        int pollTimeout = ms + 5000;
        while (!g_emulation->isDebuggerActive()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= pollTimeout) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        // Phase 3: clean up timer and read result.
        const bool ok3 = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU unavailable after run"; return; }

            DebugSession* session = p->getDebugSession();
            bool timerDidFire = session && session->isTimerFired();
            if (session)
                session->cleanupTimer();

            result["stoppedByTimer"] = timerDidFire;
            result["breaked"]        = g_emulation->isDebuggerActive();
            AddStopInfo(p, result);
            FillCpuStateJson(cpu, result);
        });

        if (!ok3) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: emu_load
    // ================================================================
    json Tool_EmuLoad(const json& args)
    {
        if (!args.contains("file") || !args["file"].is_string())
            throw std::runtime_error("file (string) is required");

        std::string path = args["file"].get<std::string>();
        bool autorun = true;
        bool breakAfterLoad = false;
        if (args.contains("autorun") && args["autorun"].is_boolean())
            autorun = args["autorun"].get<bool>();
        if (args.contains("break_after_load") && args["break_after_load"].is_boolean())
            breakAfterLoad = args["break_after_load"].get<bool>();

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            if (!p->loadFile(path, autorun)) {
                err = "failed to load file";
                return;
            }

            result["loaded"]  = true;
            result["file"]    = path;
            result["autorun"] = autorun;

            if (autorun && breakAfterLoad) {
                CpuDebugTarget* cpu = GetTarget(p);
                DebugSession* session = p->getDebugSession();
                if (cpu && session) {
                    session->requestBreak(DC_EXTERNAL);
                    FillCpuStateJson(cpu, result);
                }
                result["breaked"] = g_emulation->isDebuggerActive();
            }
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: kbd_send
    // ================================================================
    constexpr int KBD_TAP_MAX_MS = 60000;

    json Tool_KbdSend(const json& args)
    {
        if (!args.contains("keys") || !args["keys"].is_array() || args["keys"].empty())
            throw std::runtime_error("keys (non-empty array of strings) is required");

        std::vector<EmuKey> keys;
        for (auto& k : args["keys"]) {
            if (!k.is_string())
                throw std::runtime_error("keys must be strings");
            std::string name = k.get<std::string>();
            for (char& c : name) c = static_cast<char>(std::tolower(c));
            keys.push_back(ParseKeyName(name));
        }

        std::string action = "tap";
        if (args.contains("action") && args["action"].is_string())
            action = args["action"].get<std::string>();
        if (action != "press" && action != "release" && action != "tap")
            throw std::runtime_error("invalid action (expected press|release|tap)");

        int durationMs = 100;
        if (args.contains("duration_ms") && !args["duration_ms"].is_null()) {
            const json& t = args["duration_ms"];
            if (t.is_number()) durationMs = t.get<int>();
            else if (t.is_string()) durationMs = std::stoi(t.get<std::string>());
            if (durationMs < 10 || durationMs > KBD_TAP_MAX_MS)
                throw std::runtime_error("duration_ms must be between 10 and " + std::to_string(KBD_TAP_MAX_MS));
        }

        json        result;
        std::string err;
        result["action"] = action;
        result["keys"]   = args["keys"];

        // --- press / release: immediate ---
        if (action == "press" || action == "release") {
            bool isPressed = (action == "press");
            const bool ok = mcp::Run([&] {
                Platform* p = GetPlatform();
                if (!p) { err = "no platform created"; return; }
                Keyboard* kb = p->getKeyboard();
                if (!kb) { err = "no keyboard found"; return; }
                for (auto k : keys)
                    kb->processKey(k, isPressed);
            });
            if (!ok) throw std::runtime_error("emulator main thread is not ready");
            if (!err.empty()) throw std::runtime_error(err);
            return result;
        }

        // --- tap: press, hold, release ---
        DebugElapsedTimer* kbdTimer = nullptr;

        // Phase 1: press keys, set up emulation timer, run if breaked.
        const bool ok1 = mcp::Run([&] {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            Cpu* cpu = p->getCpu();
            if (!cpu) { err = "CPU not available"; return; }
            Keyboard* kb = p->getKeyboard();
            if (!kb) { err = "no keyboard found"; return; }

            DebugSession* session = p->getDebugSession();
            if (!session) { err = "debugger not available in MCP mode"; return; }

            for (auto k : keys)
                kb->processKey(k, true);

            // the stop on the timer is MCP's own, the debug window doesn't show up
            kbdTimer = new DebugElapsedTimer(session, DC_EXTERNAL);
            kbdTimer->start(static_cast<unsigned>(durationMs));

            result["wasBreaked"] = g_emulation->isDebuggerActive();
            if (g_emulation->isDebuggerActive())
                session->run(DC_EXTERNAL);
        });
        if (!ok1) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);

        bool wasBreaked = result["wasBreaked"].get<bool>();

        // Phase 2: wait for emulation timer or breakpoint.
        while (!g_emulation->isDebuggerActive())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // Phase 3: release keys, cleanup timer, preserve breaked state.
        const bool ok3 = mcp::Run([&] {
            Platform* p = GetPlatform();
            if (!p) return;

            for (auto k : keys)
                if (Keyboard* kb = p->getKeyboard())
                    kb->processKey(k, false);

            bool timerFired = kbdTimer && kbdTimer->isPaused();
            if (kbdTimer) {
                kbdTimer->stop();
                delete kbdTimer;
                kbdTimer = nullptr;
            }

            // Timer fires → debugRequest stops CPU. Only a BP stops it otherwise.
            bool stoppedByBreakpoint = g_emulation->isDebuggerActive() && !timerFired;

            // Restore CPU state after timer intervention.
            if (timerFired) {
                // was breaked: stay stopped, otherwise continue
                DebugSession* session = p->getDebugSession();
                if (!wasBreaked && session)
                    session->run(DC_EXTERNAL);
            }

            result["breaked"]             = g_emulation->isDebuggerActive();
            result["stoppedByBreakpoint"] = stoppedByBreakpoint;
            result["completed"]           = !stoppedByBreakpoint;
            result["duration_ms"]         = durationMs;
            AddStopInfo(p, result);
        });
        if (!ok3) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: kbd_reset
    // ================================================================
    json Tool_KbdReset(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&] {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            Keyboard* kb = p->getKeyboard();
            if (kb) kb->resetKeys();
            KbdTapper* kt = p->getKbdTapper();
            if (kt) kt->stopTyping();
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: mouse
    // ================================================================
    json Tool_Mouse(const json& args)
    {
        json        result;
        std::string err;

        auto checkRange = [](const char* name, long long v, int lo, int hi) {
            if (v < lo || v > hi)
                throw std::runtime_error(std::string(name) + " must be between " + std::to_string(lo) + " and " + std::to_string(hi));
        };
        auto optInt = [&](const char* name, int& v, int lo, int hi) {
            if (!args.contains(name) || args[name].is_null()) return false;
            v = ParseNumArg(args[name], name);
            checkRange(name, v, lo, hi);
            return true;
        };
        int x = -1, y = -1, buttons = -1, wheel = 0;
        const bool hasX = optInt("x", x, 0, 511), hasY = optInt("y", y, 0, 255), hasB = optInt("buttons", buttons, 0, 7);
        // wheel бывает отрицательным, поэтому ParseNumArg (беззнаковый) применяется к строке без знака
        if (args.contains("wheel") && !args["wheel"].is_null()) {
            const json& w = args["wheel"];
            long long v;
            if (w.is_number_integer()) {
                v = w.get<long long>();
            } else if (w.is_string()) {
                std::string s = w.get<std::string>();
                const size_t b = s.find_first_not_of(" \t");
                const bool neg = b != std::string::npos && s[b] == '-';
                v = ParseNumArg(json(neg ? s.substr(b + 1) : s), "wheel");
                if (neg)
                    v = -v;
            } else {
                throw std::runtime_error("invalid wheel value");
            }
            checkRange("wheel", v, -127, 127);
            wheel = int(v);
        }
        if (hasX != hasY)
            throw std::runtime_error("x and y go together");

        const bool ok = mcp::Run([&] {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            MouseDevice* m = p->getMouse();
            if (!m) { err = "this platform has no mouse or it is disabled in the platform settings"; return; }
            if (hasX && x > m->emuMouseMaxX()) {
                err = "x must be between 0 and " + std::to_string(m->emuMouseMaxX()) + " for this platform";
                return;
            }
            if (hasY && y > m->emuMouseMaxY()) {
                err = "y must be between 0 and " + std::to_string(m->emuMouseMaxY()) + " for this platform";
                return;
            }
            if (hasX || hasB)
                m->emuMouseSet(x, y, buttons);   // -1 — оставить как есть
            if (wheel)
                m->hostMouseWheel(wheel);
            result["state"] = m->getMouseInfo();
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: kbd_text
    // ================================================================
    json Tool_KbdText(const json& args)
    {
        if (!args.contains("text") || !args["text"].is_string())
            throw std::runtime_error("text (string) is required");

        std::string text = args["text"].get<std::string>();
        if (text.empty())
            throw std::runtime_error("text must be non-empty");

        json        result;
        std::string err;
        result["text"] = text;

        // Phase 1: start typing.
        const bool ok1 = mcp::Run([&] {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            KbdTapper* kt = p->getKbdTapper();
            if (!kt) { err = "KbdTapper not available on this platform"; return; }

            kt->typeText(text);

            result["wasBreaked"] = g_emulation->isDebuggerActive();
            if (g_emulation->isDebuggerActive()) {
                DebugSession* session = p->getDebugSession();
                if (!session) { err = "debugger not available in MCP mode"; return; }
                session->run(DC_EXTERNAL);
            }
        });
        if (!ok1) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);

        // Phase 2: wait for typing to complete or breakpoint.
        bool stillTyping = true;
        while (stillTyping && !g_emulation->isDebuggerActive()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            const bool ok = mcp::Run([&] {
                Platform* p = GetPlatform();
                if (!p) return;
                KbdTapper* kt = p->getKbdTapper();
                if (!kt || !kt->isTyping())
                    stillTyping = false;
            });
            if (!ok) break;
        }

        bool stoppedByBreakpoint = g_emulation->isDebuggerActive();
        bool wasBreaked = result["wasBreaked"].get<bool>();

        // Phase 3: restore breaked state if needed, read result.
        const bool ok3 = mcp::Run([&] {
            Platform* p = GetPlatform();
            if (!p) return;

            // If was breaked and typing completed (not BP), return to breaked.
            if (wasBreaked && !stoppedByBreakpoint) {
                DebugSession* session = p->getDebugSession();
                if (session)
                    session->requestBreak(DC_EXTERNAL);
            }

            result["completed"]           = !stoppedByBreakpoint;
            result["stoppedByBreakpoint"] = stoppedByBreakpoint;
            result["breaked"]             = g_emulation->isDebuggerActive();
            AddStopInfo(p, result);
        });
        if (!ok3) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: debug_break
    // ================================================================
    json Tool_DebugBreak(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }

            const bool wasBreaked = g_emulation->isDebuggerActive();
            if (!wasBreaked) {
                DebugSession* session = p->getDebugSession();
                if (!session) { err = "debugger not available in MCP mode"; return; }
                session->requestBreak(DC_EXTERNAL);
            }

            uint16_t pc = cpu->getPC();
            result["breaked"]  = g_emulation->isDebuggerActive();
            result["changed"]  = !wasBreaked;
            result["pc"]       = Hex(pc);

            // Disassemble instruction at PC
            if (g_emulation->isDebuggerActive())
                result["instruction"] = InstrText(cpu, pc);
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: debug_step
    // ================================================================
    constexpr int STEP_TIMEOUT_MS = 5000;

    json Tool_DebugStep(const json& args)
    {
        std::string mode = "into";
        if (args.contains("mode") && args["mode"].is_string())
            mode = args["mode"].get<std::string>();
        if (mode != "into" && mode != "over" && mode != "out" && mode != "skip")
            throw std::runtime_error("invalid mode (expected into|over|out|skip)");

        int timeoutMs = STEP_TIMEOUT_MS;
        if (args.contains("timeout_ms") && !args["timeout_ms"].is_null()) {
            const json& t = args["timeout_ms"];
            if (t.is_number())
                timeoutMs = t.get<int>();
            else if (t.is_string())
                timeoutMs = std::stoi(t.get<std::string>());
            if (timeoutMs < 1 || timeoutMs > 60000)
                throw std::runtime_error("timeout_ms must be between 1 and 60000");
        }

        json        result;
        std::string err;
        bool        async = false;

        result["mode"] = mode;

        // Phase 1: trigger the step on the main thread.
        const bool ok1 = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }

            if (!g_emulation->isDebuggerActive()) {
                err = "CPU is not paused; call debug_break first";
                return;
            }

            DebugSession* session = p->getDebugSession();
            if (!session) { err = "debugger not available in MCP mode"; return; }

            if (mode == "skip") {
                uint16_t pc = cpu->getPC();
                session->setPC(static_cast<uint16_t>(pc + cpu->getInstrLength(pc)));
                return;
            }

            if (mode == "into")
                session->stepInto(DC_EXTERNAL);
            else if (mode == "over")
                session->stepOver(DC_EXTERNAL);
            else // "out"
                session->stepOut(DC_EXTERNAL);

            async = true;
        });

        if (!ok1) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);

        // Phase 2: wait for re-break.
        bool completed = true;
        if (async) {
            auto start = std::chrono::steady_clock::now();
            while (!g_emulation->isDebuggerActive()) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start).count();
                if (elapsed >= timeoutMs) break;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            completed = g_emulation->isDebuggerActive();
        }

        // Phase 3: read result.
        const bool ok3 = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU unavailable after step"; return; }

            uint16_t pc = cpu->getPC();
            result["breaked"]   = g_emulation->isDebuggerActive();
            result["completed"] = completed;
            result["pc"]        = Hex(pc);
            AddStopInfo(p, result);

            if (g_emulation->isDebuggerActive())
                result["instruction"] = InstrText(cpu, pc);
        });

        if (!ok3) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: emu_reset
    // ================================================================
    json Tool_EmuReset(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU is not available"; return; }

            p->reset();
            result["reset"]   = true;
            result["breaked"] = g_emulation->isDebuggerActive();
            result["pc"]      = Hex(cpu->getPC());
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: switch_platform
    // ================================================================
    json Tool_SwitchPlatform(const json& args)
    {
        if (!args.contains("platform") || !args["platform"].is_string())
            throw std::runtime_error("platform (string) is required");

        std::string target = args["platform"].get<std::string>();

        json        result;
        std::string err;

        const bool ok = mcp::Run([&] {
            Platform* p = GetPlatform();
            std::string curName = p ? p->getBaseName() : "(none)";
            result["previous"] = curName;

            if (target == curName) {
                result["switched"] = false;
                result["note"] = "already on this platform";
                return;
            }

            // An unknown name would leave the emulator without a platform, and it would exit
            bool known = false;
            if (EmuConfig* ec = g_emulation->getConfig())
                for (const auto& pi : *ec->getPlatformInfos())
                    if (pi.objName == target)
                        known = true;
            if (!known) {
                err = "unknown platform '" + target + "', see 'platform' in sys_info's available_platforms";
                return;
            }

            emuSelectPlatform(target);
            Platform* newP = GetPlatform();
            std::string newName = newP ? newP->getBaseName() : "(none)";
            result["switched"] = (newName == target);
            result["current"]  = newName;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Breakpoint type names used by the bp_* tools
    // ================================================================
    const char* const BP_TYPES_EXPECTED = "exec|write|read|access|port_read|port_write|port_access";

    bool ParseBpType(const std::string& name, BreakpointType& type)
    {
        static const struct { const char* name; BreakpointType type; } types[] = {
            { "exec",        BT_EXEC },
            { "write",       BT_WRITE },
            { "read",        BT_READ },
            { "access",      BT_ACSESS },
            { "port_write",  BT_PORT_WRITE },
            { "port_read",   BT_PORT_READ },
            { "port_access", BT_PORT_ACSESS },
        };
        for (auto& t : types)
            if (name == t.name) { type = t.type; return true; }
        return false;
    }

    const char* BpTypeName(BreakpointType type)
    {
        switch (type) {
        case BT_EXEC:        return "exec";
        case BT_WRITE:       return "write";
        case BT_READ:        return "read";
        case BT_ACSESS:      return "access";
        case BT_PORT_WRITE:  return "port_write";
        case BT_PORT_READ:   return "port_read";
        case BT_PORT_ACSESS: return "port_access";
        }
        return "unknown";
    }

    // ================================================================
    // Tool: bp_list
    // ================================================================
    json Tool_BpList(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            DebugSession* session = p->getDebugSession();
            if (!session) { err = "debugger not available in MCP mode"; return; }

            std::vector<BreakpointDesc> bps;
            session->getBreakpoints()->list(bps);

            // exec, then memory, then port breakpoints
            json items = json::array();
            int idx = 0;
            for (BreakpointClass bc : { BC_EXEC, BC_MEM, BC_IO })
                for (auto& bp : bps) {
                    if (getBreakpointClass(bp.type) != bc)
                        continue;
                    items.push_back({
                        { "index",     idx++ },
                        { "id",        bp.id },
                        { "enabled",   bp.enabled },
                        { "owner",     bp.owner == DC_EXTERNAL ? "mcp" : "ui" },
                        { "addr",      Hex(bp.addr) },
                        { "type",      BpTypeName(bp.type) },
                        { "hitCount",  bp.hitCount },
                        { "skipCount", bp.skipCount },
                        { "remaining", bp.remaining },
                        { "comment",   bp.comment },
                    });
                }
            result["count"]       = idx;
            result["breakpoints"] = items;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: bp_set
    // ================================================================
    json Tool_BpSet(const json& args)
    {
        if (!args.contains("addr") || args["addr"].is_null())
            throw std::runtime_error("addr is required");

        const uint16_t addr = ParseNumArg(args["addr"], "addr");
        std::string type = "exec";
        if (args.contains("type") && args["type"].is_string())
            type = args["type"].get<std::string>();
        BreakpointType bt;
        if (!ParseBpType(type, bt))
            throw std::runtime_error(std::string("invalid type (expected ") + BP_TYPES_EXPECTED + ")");

        int skipCount = -1; // -1 means "don't set"
        if (args.contains("skipCount") && args["skipCount"].is_number_integer())
            skipCount = args["skipCount"].get<int>();

        std::string comment;
        bool hasComment = args.contains("comment") && args["comment"].is_string();
        if (hasComment)
            comment = args["comment"].get<std::string>();

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            DebugSession* session = p->getDebugSession();
            if (!session) { err = "debugger not available in MCP mode"; return; }
            BreakpointManager* bps = session->getBreakpoints();

            bool exists = bps->find(bt, addr) >= 0;
            int id = bps->add(bt, addr, DC_EXTERNAL);
            if (id < 0) { err = "breakpoint type '" + type + "' is not supported by this CPU"; return; }
            if (skipCount >= 0)
                bps->setSkipCount(id, skipCount);
            if (hasComment)
                bps->setComment(id, comment);

            result["added"] = !exists;
            result["id"]    = id;
            result["addr"]  = Hex(addr);
            result["type"]  = type;
            if (skipCount >= 0)
                result["skipCount"] = skipCount;
            if (hasComment)
                result["comment"] = comment;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: bp_remove
    // ================================================================
    json Tool_BpRemove(const json& args)
    {
        const bool hasId = args.contains("id") && args["id"].is_number_integer();
        if (!hasId && (!args.contains("addr") || args["addr"].is_null()))
            throw std::runtime_error("addr or id is required");

        const int id = hasId ? args["id"].get<int>() : -1;
        const uint16_t addr = hasId ? 0 : ParseNumArg(args["addr"], "addr");

        // Optional type filter — if provided, only remove matching BPs.
        std::string type;
        BreakpointType bt = BT_EXEC;
        bool hasType = args.contains("type") && args["type"].is_string();
        if (hasType) {
            type = args["type"].get<std::string>();
            if (!ParseBpType(type, bt))
                throw std::runtime_error(std::string("invalid type (expected ") + BP_TYPES_EXPECTED + ")");
        }

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            DebugSession* session = p->getDebugSession();
            if (!session) {
                result["removed"] = 0;
                result["count"]   = 0;
                return;
            }
            BreakpointManager* bps = session->getBreakpoints();

            std::vector<BreakpointDesc> descs;
            bps->list(descs);

            int removed = 0;
            for (auto& bp : descs) {
                bool match = hasId ? bp.id == id : bp.addr == addr && (!hasType || bp.type == bt);
                if (match && bps->remove(bp.id))
                    ++removed;
            }

            result["removed"] = removed;
            if (hasId)
                result["id"] = id;
            else
                result["addr"] = Hex(addr);
            if (hasType)
                result["type"] = type;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: bp_clear
    // ================================================================
    json Tool_BpClear(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            DebugSession* session = p->getDebugSession();
            if (!session) {
                result["removed"] = 0;
                result["count"]   = 0;
                return;
            }
            BreakpointManager* bps = session->getBreakpoints();

            std::vector<BreakpointDesc> descs;
            bps->list(descs);
            bps->clear();

            result["cleared"] = static_cast<int>(descs.size());
            result["count"]   = 0;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // Tool: state_save (deferred — not yet implemented)
    // json Tool_StateSave(const json&) { throw std::runtime_error("not implemented"); }

    // Tool: state_load (deferred — not yet implemented)
    // json Tool_StateLoad(const json&) { throw std::runtime_error("not implemented"); }

    // ================================================================
    // Tool: disk_attach
    // ================================================================
    json Tool_DiskAttach(const json& args)
    {
        if (!args.contains("drive") || !args["drive"].is_string())
            throw std::runtime_error("drive (A-D or HDD) is required");
        std::string drive = args["drive"].get<std::string>();
        for (char& c : drive) c = static_cast<char>(std::toupper(c));
        bool isHdd = (drive == "HDD");
        if (!isHdd && (drive.size() != 1 || drive[0] < 'A' || drive[0] > 'D'))
            throw std::runtime_error("invalid drive (expected A, B, C, D or HDD)");

        if (!args.contains("file") || !args["file"].is_string())
            throw std::runtime_error("file (string) is required");
        std::string fileName = args["file"].get<std::string>();

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            std::string propName = isHdd ? std::string(".hdd") : std::string(".disk") + drive[0];
            bool success = emuSetPropertyValue(p->getName() + propName, "fileName", fileName);

            result["attached"] = success;
                result["drive"]    = drive;
            if (success) {
                palUpdateConfig();
                result["file"]     = fileName;
            }
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: disk_detach
    // ================================================================
    json Tool_DiskDetach(const json& args)
    {
        if (!args.contains("drive") || !args["drive"].is_string())
            throw std::runtime_error("drive (A-D or HDD) is required");
        std::string drive = args["drive"].get<std::string>();
        for (char& c : drive) c = static_cast<char>(std::toupper(c));
        bool isHdd = (drive == "HDD");
        if (!isHdd && (drive.size() != 1 || drive[0] < 'A' || drive[0] > 'D'))
            throw std::runtime_error("invalid drive (expected A, B, C, D or HDD)");

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            std::string propName = isHdd ? std::string(".hdd") : std::string(".disk") + drive[0];
            bool success = !emuSetPropertyValue(p->getName() + propName, "fileName", "");

            result["detached"] = success;
            if (success) {
                palUpdateConfig();
                result["drive"] = drive;
            }
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: mem_devices
    // ================================================================
    json Tool_MemDevices(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            DebugSession* session = p->getDebugSession();
            if (!session) { err = "CPU is not available"; return; }

            json items = json::array();
            for (const DebugMemory& mem : session->getMemories())
                items.push_back({ { "device", mem.name }, { "size", Hex32(mem.size) }, { "size_dec", mem.size } });
            result["devices"] = items;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: devices_info
    // ================================================================
    json Tool_DevicesInfo(const json& /*args*/)
    {
        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            DebugSession* session = p->getDebugSession();
            if (!session) { err = "CPU is not available"; return; }
            json items = json::array();
            for (const DebugDeviceInfo& dev : session->getDevicesInfo())
                items.push_back({ { "object", dev.name }, { "info", dev.info } });
            result["devices"] = items;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: emu_run_to
    // ================================================================
    json Tool_EmuRunTo(const json& args)
    {
        if (!args.contains("addr") || args["addr"].is_null())
            throw std::runtime_error("addr is required");
        const uint16_t addr = ParseNumArg(args["addr"], "addr");

        int timeoutMs = STEP_TIMEOUT_MS;
        if (args.contains("timeout_ms") && args["timeout_ms"].is_number_integer()) {
            timeoutMs = args["timeout_ms"].get<int>();
            if (timeoutMs < 1 || timeoutMs > 60000)
                throw std::runtime_error("timeout_ms must be between 1 and 60000");
        }

        json        result;
        std::string err;

        // Phase 1: run to the address
        const bool ok1 = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            DebugSession* session = p->getDebugSession();
            if (!session) { err = "CPU is not available"; return; }
            if (!g_emulation->isDebuggerActive()) {
                err = "CPU is not paused; call debug_break first";
                return;
            }
            session->runTo(addr, DC_EXTERNAL);
        });
        if (!ok1) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);

        // Phase 2: wait for a stop
        auto start = std::chrono::steady_clock::now();
        while (!g_emulation->isDebuggerActive()) {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeoutMs) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        // Phase 3: read result
        const bool ok3 = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            CpuDebugTarget* cpu = GetTarget(p);
            if (!cpu) { err = "CPU unavailable after run"; return; }

            const bool breaked = g_emulation->isDebuggerActive();
            const uint16_t pc = cpu->getPC();
            result["breaked"] = breaked;
            result["reached"] = breaked && pc == addr;
            result["pc"]      = Hex(pc);
            AddStopInfo(p, result);
            if (breaked)
                result["instruction"] = InstrText(cpu, pc);
        });
        if (!ok3) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool: bp_enable
    // ================================================================
    json Tool_BpEnable(const json& args)
    {
        if (!args.contains("id") || !args["id"].is_number_integer())
            throw std::runtime_error("id (integer, see bp_list) is required");
        const int id = args["id"].get<int>();
        const bool enabled = OptBool(args, "enabled", true);

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }
            DebugSession* session = p->getDebugSession();
            if (!session) { err = "CPU is not available"; return; }
            if (!session->getBreakpoints()->setEnabled(id, enabled)) {
                err = "no breakpoint with id " + std::to_string(id);
                return;
            }
            result["id"]      = id;
            result["enabled"] = enabled;
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return result;
    }

    // ================================================================
    // Tool registry builder
    // ================================================================
    std::vector<McpTool> BuildToolRegistry()
    {
        std::vector<McpTool> tools;

        tools.push_back({
            "cpu_state",
            "Read all CPU registers (AF, BC, DE, HL, SP, PC) with flags (S/Z/H/P/N/C), "
            "run/break/pause state, and the instruction at PC. Supports 8080 and Z80; "
            "Z80 alternate registers (AF2/BC2/DE2/HL2/IX/IY/IR/IM) are included when "
            "applicable. Hex values are the default format, with decimal equivalents "
            "provided for each register. When stopped, 'stop_reason' tells why "
            "(break, breakpoint with 'breakpoint_id', step, timer, halt, illegal).",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_CpuState
        });

        {
            json props = json::object();
            const json regSchema = { { "type", json::array({ "string", "integer" }) } };
            for (const char* r : { "af", "bc", "de", "hl", "sp", "pc", "psw", "iff",
                                   "af2", "bc2", "de2", "hl2", "ix", "iy", "i", "r", "im" })
                props[r] = regSchema;

            tools.push_back({
                "regs_set",
                "Set one or more CPU registers in one call. Provide any subset of "
                "af/bc/de/hl/sp/pc/psw/iff (all CPUs) or af2/bc2/de2/hl2/ix/iy/i/r/im "
                "(Z80 only); single flags can be set by name too (8080: s/z/ac/p/c, "
                "Z80: s/z/h/p/n/c). Returns all registers. Number format: JSON integer = decimal, JSON string = hex "
                "by default (0x=hex, 0o=octal, leading 0=decimal). "
                "Requires CPU to be paused.",
                json{ { "type", "object" }, { "properties", props } },
                &Tool_RegsSet
            });
        }

        tools.push_back({
            "sys_info",
            "Report environment info: MCP server identity, current platform name, "
            "CPU type (8080/Z80), clock speed, run/break state, emulation speed "
            "factor, and list of all available platforms.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_SysInfo
        });

        tools.push_back({
            "disasm",
            "Disassemble a run of instructions. 'addr' is start address "
            "(JSON integer = decimal, JSON string = hex by default, "
            "0x=hex, 0o=octal, leading 0=decimal); if omitted, starts at PC. "
            "'count' is number of instructions (default 16, max 64). "
            "Returns instruction text, raw bytes, and addresses.",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr",  { { "type", json::array({ "string", "integer" }) }, { "description", "start address; default = PC" } } },
                    { "count", { { "type", "integer" }, { "minimum", 1 }, { "maximum", DISASM_MAX_COUNT }, { "description", "count (default 16)" } } },
                }}
            },
            &Tool_Disasm
        });

        tools.push_back({
            "mem_read",
            "Read a range of memory. 'addr' is required "
            "(JSON integer = decimal, JSON string = hex by default, "
            "0x=hex, 0o=octal, leading 0=decimal). "
            "'len' is byte count (default 64, max 4096). Optional 'device' selects a "
            "memory from mem_devices (default: the CPU address space). Returns compact "
            "hex dump and ASCII rendering (non-printable → '.').",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr", { { "type", json::array({ "string", "integer" }) }, { "description", "start address (required)" } } },
                    { "device", { { "type", "string" }, { "description", "memory name from mem_devices (default cpu)" } } },
                    { "len",  { { "type", "integer" }, { "minimum", 1 }, { "maximum", MEM_READ_MAX_LEN }, { "description", "byte count (default 64)" } } },
                }},
                { "required", json::array({ "addr" }) },
            },
            &Tool_MemRead
        });

        tools.push_back({
            "mem_write",
            "Write bytes to memory. 'addr' is required "
            "(JSON integer = decimal, JSON string = hex by default, "
            "0x=hex, 0o=octal, leading 0=decimal). "
            "'data' is required — a hex string of bytes to write (e.g. \"C3 00 01\" "
            "or \"C30001\"). Non-hex characters are stripped. Max 4096 bytes. "
            "Optional 'device' selects a memory from mem_devices (default: the CPU "
            "address space).",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr", { { "type", json::array({ "string", "integer" }) }, { "description", "start address (required)" } } },
                    { "device", { { "type", "string" }, { "description", "memory name from mem_devices (default cpu)" } } },
                    { "data", { { "type", "string" }, { "description", "hex bytes to write (required)" } } },
                }},
                { "required", json::array({ "addr", "data" }) },
            },
            &Tool_MemWrite
        });

        tools.push_back({
            "mem_devices",
            "List memories that mem_read/mem_write can access with the 'device' "
            "argument: 'cpu' (the CPU address space, the default) and the platform's "
            "RAM/ROM objects and other devices marked for the debugger, with sizes.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_MemDevices
        });

        tools.push_back({
            "devices_info",
            "Debug information of the platform devices that provide it (DMA, CRT, "
            "PIC, FDC etc.), one text block per device object.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_DevicesInfo
        });

        tools.push_back({
            "port_read",
            "Read a single byte from an I/O port. 'port' (0–255) is required. "
            "Number format: JSON integer = decimal, JSON string = hex by default.",
            json{
                { "type", "object" },
                { "properties", {
                    { "port", { { "type", json::array({ "string", "integer" }) }, { "description", "port number 0–255 (required)" } } },
                }},
                { "required", json::array({ "port" }) },
            },
            &Tool_PortRead
        });

        tools.push_back({
            "port_write",
            "Write a single byte to an I/O port. 'port' (0–255) and 'value' "
            "(byte) are required. Number format: JSON integer = decimal, "
            "JSON string = hex by default.",
            json{
                { "type", "object" },
                { "properties", {
                    { "port",  { { "type", json::array({ "string", "integer" }) }, { "description", "port number 0–255 (required)" } } },
                    { "value", { { "type", json::array({ "string", "integer" }) }, { "description", "value to write (required)" } } },
                }},
                { "required", json::array({ "port", "value" }) },
            },
            &Tool_PortWrite
        });

        tools.push_back({
            "screen",
            "Capture the current display as a PNG image at native resolution. "
            "Returns MCP image content block.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_Screen
        });

        tools.push_back({
            "text_screen",
            "Return the current screen contents as a text string (UTF-8). "
            "Only available on platforms with a text-mode renderer. "
            "Leading/trailing empty lines and leading whitespace are trimmed.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_TextScreen
        });

        tools.push_back({
            "kbd_send",
            "Send keyboard events. 'keys' is an array of key name strings "
            "(e.g. \"a\", \"enter\", \"f1\"). 'action' is \"press\" (press "
            "and return), \"release\" (release and return), or \"tap\" "
            "(press, hold for 'duration_ms', release; blocks until done, "
            "or until a breakpoint fires). 'duration_ms' is required for "
            "\"tap\" (10–60000).",
            json{
                { "type", "object" },
                { "properties", {
                    { "keys",        { { "type", "array" }, { "items", json{ {"type","string"} } }, { "description", "key names (required)" } } },
                    { "action",      { { "type", "string" }, { "enum", json::array({ "press", "release", "tap" }) }, { "description", "default tap" } } },
                    { "duration_ms", { { "type", "integer" }, { "minimum", 10 }, { "maximum", KBD_TAP_MAX_MS }, { "description", "for tap: hold time in ms (default 100)" } } },
                }},
                { "required", json::array({ "keys" }) },
            },
            &Tool_KbdSend
        });

        tools.push_back({
            "kbd_reset",
            "Release all virtually pressed keys.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_KbdReset
        });

        tools.push_back({
            "mouse",
            "Emulated platform mouse (Vector-06C: KTSerg controller on port D4h; Specialist: adapter on the PPI at F000h, ports F004/F005; "
            "Specialist-MX2: adapter on the second PPI, ports FFE4h/FFE5h; Korvet: Microsoft serial mouse on the i8251). The mouse is optional and off by default (CFG_MOUSE in the platform settings). "
            "'x' and 'y' (0-255, grows upward, 255 = top line) move the pointer like the host cursor over the screen. "
            "Vector: 'x' 0-511 in 512-pixel mode points (256-mode X = x/2). Specialist: 'x' 0-383, Korvet: 'x' 0-511; these mice report movement only, "
            "so x/y set a virtual pointer (the screen center after reset) and the program receives the offset from its previous position. "
            "'buttons' bit 0 left, bit 1 right, bit 2 middle, held until changed by another call and combined with the host mouse buttons; "
            "on Vector hold them until the program has read them. 'wheel' adds wheel steps. All optional; returns the controller state.",
            json{
                { "type", "object" },
                { "properties", {
                    { "x",       { { "type", json::array({ "string", "integer" }) }, { "description", "0-511 (Vector, Korvet), 0-383 (Specialist)" } } },
                    { "y",       { { "type", json::array({ "string", "integer" }) }, { "description", "0-255, upward" } } },
                    { "buttons", { { "type", json::array({ "string", "integer" }) }, { "description", "bit0 left, bit1 right, bit2 middle" } } },
                    { "wheel",   { { "type", json::array({ "string", "integer" }) }, { "description", "wheel steps -127..127, + away from user" } } },
                }},
            },
            &Tool_Mouse
        });

        tools.push_back({
            "kbd_text",
            "Type a text string into the emulated machine. 'text' is required. "
            "Blocks until typing completes or a breakpoint fires. Only available "
            "on platforms that have a KbdTapper configured.",
            json{
                { "type", "object" },
                { "properties", {
                    { "text", { { "type", "string" }, { "description", "text to type (required)" } } },
                }},
                { "required", json::array({ "text" }) },
            },
            &Tool_KbdText
        });

        tools.push_back({
            "emu_load",
            "Load a file into the emulated machine. 'file' is required. "
            "Optional 'autorun' (bool, default true) auto-starts after load. "
            "Optional 'break_after_load' (bool, default false) breaks into "
            "debugger right after load if autorun is true, and includes "
            "full CPU state in the response.",
            json{
                { "type", "object" },
                { "properties", {
                    { "file",             { { "type", "string" },  { "description", "file path (required)" } } },
                    { "autorun",          { { "type", "boolean" }, { "description", "auto-start after load (default true)" } } },
                    { "break_after_load", { { "type", "boolean" }, { "description", "break into debugger after load (default false)" } } },
                }},
                { "required", json::array({ "file" }) },
            },
            &Tool_EmuLoad
        });

        tools.push_back({
            "emu_run",
            "Resume free-running execution: leave the debug break if paused, or "
            "start the CPU if it was stopped. Returns resulting run/break state "
            "and the current PC.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_EmuRun
        });

        tools.push_back({
            "emu_run_for",
            "Run the CPU for the specified number of milliseconds and then stop. "
            "'ms' (1–60000) is required. Stops early if a breakpoint fires. "
            "The timer resets if a breakpoint stops the CPU before the timeout. ",
            json{
                { "type", "object" },
                { "properties", {
                    { "ms", { { "type", "integer" }, { "minimum", 1 }, { "maximum", RUN_FOR_MAX_MS }, { "description", "duration in milliseconds" } } },
                }},
                { "required", json::array({ "ms" }) },
            },
            &Tool_EmuRunFor
        });

        tools.push_back({
            "debug_break",
            "Pause the CPU into the debugger at the current PC. Also exits "
            "paused state if the emulator was paused. Idempotent. "
            "Returns resulting state and the instruction at the PC.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_DebugBreak
        });

        tools.push_back({
            "debug_step",
            "Single-step the paused CPU. 'mode': 'into' (one instruction; default), "
            "'over' (step over calls), 'out' (run until the current subroutine "
            "returns), or 'skip' (advance PC without executing). "
            "Optional 'timeout_ms' (1–60000, default 5000) limits wait for step "
            "completion. Requires CPU paused. Returns new PC, instruction and "
            "stop_reason ('step', or 'breakpoint' with 'breakpoint_id' if one fired first).",
            json{
                { "type", "object" },
                { "properties", {
                    { "mode",       { { "type", "string" }, { "enum", json::array({ "into", "over", "out", "skip" }) }, { "description", "step mode (default into)" } } },
                    { "timeout_ms", { { "type", "integer" }, { "minimum", 1 }, { "maximum", 60000 }, { "description", "timeout in ms (default 5000)" } } },
                }}
            },
            &Tool_DebugStep
        });

        tools.push_back({
            "emu_run_to",
            "Run the paused CPU until it reaches 'addr' (a temporary breakpoint; "
            "running to the current PC means a full loop). Other breakpoints may "
            "stop it earlier ('reached' is false then). Optional 'timeout_ms' "
            "(1–60000, default 5000) limits the wait; on timeout the CPU keeps running.",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr",       { { "type", json::array({ "string", "integer" }) }, { "description", "address to run to (required)" } } },
                    { "timeout_ms", { { "type", "integer" }, { "minimum", 1 }, { "maximum", 60000 }, { "description", "timeout in ms (default 5000)" } } },
                }},
                { "required", json::array({ "addr" }) },
            },
            &Tool_EmuRunTo
        });

        tools.push_back({
            "emu_reset",
            "Reset the emulated machine (cold reset). Returns post-reset state "
            "and the PC at the reset vector.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_EmuReset
        });

        tools.push_back({
            "switch_platform",
            "Switch to a different emulated platform. 'platform' is the platform "
            "name from sys_info's available_platforms. Returns previous and current "
            "platform name.",
            json{
                { "type", "object" },
                { "properties", {
                    { "platform", { { "type", "string" }, { "description", "platform name from available_platforms (required)" } } },
                }},
                { "required", json::array({ "platform" }) },
            },
            &Tool_SwitchPlatform
        });

        tools.push_back({
            "bp_list",
            "List all breakpoints with index, id, enabled, owner (ui/mcp), address (hex), type, hitCount (total hits "
            "since the breakpoint was set), skipCount, remaining (hits to skip before "
            "the next stop) and comment.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_BpList
        });

        tools.push_back({
            "bp_set",
            "Add or configure a breakpoint. 'addr' is required "
            "(JSON integer = decimal, JSON string = hex by default, "
            "0x=hex, 0o=octal, leading 0=decimal). "
            "'type' is \"exec\" (code execution, default), \"write\" / \"read\" "
            "/ \"access\" (memory), or \"port_write\" / \"port_read\" / "
            "\"port_access\" (I/O ports). "
            "Optional 'skipCount' makes the breakpoint stop on every "
            "(skipCount+1)-th hit (default 0 = stop on every hit). "
            "Duplicates of the same type+addr are a no-op (but skipCount "
            "is still applied if provided).",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr",     { { "type", json::array({ "string", "integer" }) }, { "description", "breakpoint address (required)" } } },
                    { "type",     { { "type", "string" }, { "enum", json::array({ "exec", "write", "read", "access", "port_read", "port_write", "port_access" }) }, { "description", "breakpoint type (default exec)" } } },
                    { "skipCount", { { "type", "integer" }, { "minimum", 0 }, { "description", "stop on every (N+1)-th hit (default 0)" } } },
                    { "comment",   { { "type", "string" }, { "description", "optional comment stored with the breakpoint" } } },
                }},
                { "required", json::array({ "addr" }) },
            },
            &Tool_BpSet
        });

        tools.push_back({
            "bp_remove",
            "Remove breakpoints at an address, or one breakpoint by 'id' (see bp_list). 'addr' format: "
            "JSON integer = decimal, JSON string = hex by default "
            "(0x=hex, 0o=octal, leading 0=decimal). "
            "Optional 'type' filters removal to only matching type "
            "(exec|write|read|access|port_read|port_write|port_access). "
            "Without 'type', removes all breakpoints at the address. "
            "Returns count of removed breakpoints.",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr", { { "type", json::array({ "string", "integer" }) }, { "description", "breakpoint address (required)" } } },
                    { "type", { { "type", "string" }, { "enum", json::array({ "exec", "write", "read", "access", "port_read", "port_write", "port_access" }) }, { "description", "filter by type (optional)" } } },
                    { "id",   { { "type", "integer" }, { "description", "breakpoint id instead of addr" } } },
                }},
            },
            &Tool_BpRemove
        });

        tools.push_back({
            "bp_clear",
            "Remove ALL breakpoints. Returns the number that were cleared.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_BpClear
        });

        tools.push_back({
            "bp_enable",
            "Enable or disable a breakpoint by 'id' (see bp_list). A disabled "
            "breakpoint stays in the list but does not stop or count hits.",
            json{
                { "type", "object" },
                { "properties", {
                    { "id",      { { "type", "integer" }, { "description", "breakpoint id (required)" } } },
                    { "enabled", { { "type", "boolean" }, { "description", "true to enable (default), false to disable" } } },
                }},
                { "required", json::array({ "id" }) },
            },
            &Tool_BpEnable
        });

        // state_save / state_load — not yet implemented for Emu80,
        // commented out so they don't appear in tools/list.
        /*
        tools.push_back({
            "state_save",
            "Save a full emulator state snapshot to a file. NOT YET IMPLEMENTED "
            "for Emu80 — returns an error. (Planned for a future release.)",
            json{
                { "type", "object" },
                { "properties", {
                    { "path", { { "type", "string" }, { "description", "destination file path (required)" } } },
                }},
                { "required", json::array({ "path" }) },
            },
            &Tool_StateSave
        });

        tools.push_back({
            "state_load",
            "Load a full emulator state snapshot from a file. NOT YET IMPLEMENTED "
            "for Emu80 — returns an error. (Planned for a future release.)",
            json{
                { "type", "object" },
                { "properties", {
                    { "path", { { "type", "string" }, { "description", "source file path (required)" } } },
                }},
                { "required", json::array({ "path" }) },
            },
            &Tool_StateLoad
        });
        */

        tools.push_back({
            "disk_attach",
            "Mount a disk image into drive A-D or HDD. 'drive' (A-D or HDD) "
            "and 'file' are required.",
            json{
                { "type", "object" },
                { "properties", {
                    { "drive", { { "type", "string" }, { "description", "drive: A, B, C, D or HDD (required)" } } },
                    { "file",  { { "type", "string" }, { "description", "image file path (required)" } } },
                }},
                { "required", json::array({ "drive", "file" }) },
            },
            &Tool_DiskAttach
        });

        tools.push_back({
            "disk_detach",
            "Eject the disk from drive A-D or HDD. 'drive' (A-D or HDD) is required.",
            json{
                { "type", "object" },
                { "properties", {
                    { "drive", { { "type", "string" }, { "description", "drive: A, B, C, D or HDD (required)" } } },
                }},
                { "required", json::array({ "drive" }) },
            },
            &Tool_DiskDetach
        });

        return tools;
    }
} // anonymous namespace

// ======================================================================
// Server implementation
// ======================================================================

struct CMcpServer::Impl
{
    httplib::Server          svr;
    std::thread              listenThread;
    std::vector<McpTool>     tools;
    unsigned short           port = 0;
    bool                     running = false;

    // JSON-RPC helpers.
    static json RpcError(const json& id, int code, const std::string& message)
    {
        return json{
            { "jsonrpc", "2.0" },
            { "id",      id },
            { "error",   { { "code", code }, { "message", message } } }
        };
    }

    static json RpcResult(const json& id, const json& result)
    {
        return json{
            { "jsonrpc", "2.0" },
            { "id",      id },
            { "result",  result }
        };
    }

    json HandleRpc(const json& msg)
    {
        // Validate jsonrpc version.
        auto jr = msg.find("jsonrpc");
        if (jr == msg.end() || !jr->is_string() || jr->get<std::string>() != "2.0")
            return RpcError(msg.value("id", json(nullptr)), -32600, "Invalid Request");

        auto mid = msg.find("method");
        if (mid == msg.end() || !mid->is_string())
            return RpcError(msg.value("id", json(nullptr)), -32600, "Invalid Request");

        const std::string method = mid->get<std::string>();
        const json        id     = msg.value("id", json(nullptr));
        const json        params = msg.value("params", json::object());

        // Notification (no id): don't respond.
        bool isNotification = id.is_null();

        if (method == "initialize") {
            json caps;
            caps["protocolVersion"] = MCP_PROTOCOL_VERSION;
            caps["capabilities"]    = { { "tools", { { "listChanged", false } } } };
            caps["serverInfo"]      = {
                { "name",    MCP_SERVER_NAME },
                { "version", MCP_SERVER_VERSION },
            };
            return RpcResult(id, caps);
        }

        if (method == "ping")
            return RpcResult(id, json::object());

        if (method == "tools/list") {
            json list = json::array();
            for (const auto& t : tools) {
                list.push_back({
                    { "name",        t.name },
                    { "description", t.description },
                    { "inputSchema", t.inputSchema },
                });
            }
            return RpcResult(id, { { "tools", list } });
        }

        if (method == "tools/call") {
            const std::string toolName = params.value("name", "");
            const json        toolArgs = params.value("arguments", json::object());

            for (const auto& t : tools) {
                if (t.name == toolName) {
                    try {
                        json result = t.handler(toolArgs);

                        // If the handler returned _content, pass through as rich content.
                        if (result.contains("_content")) {
                            json rich;
                            rich["content"] = result["_content"];
                            return RpcResult(id, rich);
                        }

                        // Plain/text result.
                        json content = json::array({
                            { { "type", "text" }, { "text", result.dump(2) } }
                        });
                        return RpcResult(id, {
                            { "content", content },
                            { "isError", false },
                        });
                    } catch (const std::exception& ex) {
                        json content = json::array({
                            { { "type", "text" }, { "text", ex.what() } }
                        });
                        return RpcResult(id, {
                            { "content", content },
                            { "isError", true },
                        });
                    }
                }
            }
            return RpcError(id, -32601, "Method not found: " + toolName);
        }

        return RpcError(id, -32601, "Method not found: " + method);
    }
};

CMcpServer::CMcpServer() : m_pImpl(std::make_unique<Impl>()) {}
CMcpServer::~CMcpServer() { Stop(); }

bool CMcpServer::Start(unsigned short port)
{
    if (m_pImpl->running)
        return false;

    m_pImpl->tools = BuildToolRegistry();
    m_pImpl->port  = port;

    auto& impl = *m_pImpl;

    impl.svr.Post("/mcp", [&impl](const httplib::Request& req, httplib::Response& res)
    {
        res.set_header("Mcp-Session-Id", "emu80-session");

        try {
            json body = json::parse(req.body);

            if (body.is_array()) {
                // Batch request.
                json responses = json::array();
                for (auto& msg : body) {
                    if (!msg.contains("id") || msg["id"].is_null())
                        continue; // notification
                    responses.push_back(impl.HandleRpc(msg));
                }
                if (responses.empty())
                    res.status = 202;
                else {
                    res.set_content(responses.dump(), "application/json");
                    res.status = 200;
                }
            } else if (body.is_object()) {
                json rsp = impl.HandleRpc(body);
                if (body.contains("id") && !body["id"].is_null()) {
                    res.set_content(rsp.dump(), "application/json");
                    res.status = 200;
                } else {
                    res.status = 202; // notification
                }
            } else {
                res.status = 400;
                res.set_content("Invalid JSON-RPC body", "text/plain");
            }
        } catch (const json::parse_error&) {
            res.status = 400;
            res.set_content(json(impl.RpcError(json(nullptr), -32700, "Parse error")).dump(),
                           "application/json");
        }
    });

    impl.svr.Get("/mcp", [](const httplib::Request&, httplib::Response& res) {
        res.status = 405;
        res.set_content("Method Not Allowed", "text/plain");
    });

    impl.listenThread = std::thread([&impl, port] {
        impl.svr.listen("127.0.0.1", port);
    });

    // Wait briefly for the listener to start.
    impl.svr.wait_until_ready();
    if (!impl.svr.is_running()) {
        impl.svr.stop();
        if (impl.listenThread.joinable())
            impl.listenThread.join();
        return false;
    }

    impl.running = true;
    return true;
}

void CMcpServer::Stop()
{
    if (!m_pImpl->running)
        return;

    m_pImpl->svr.stop();
    if (m_pImpl->listenThread.joinable())
        m_pImpl->listenThread.join();
    m_pImpl->running = false;
}

bool CMcpServer::IsRunning() const
{
    return m_pImpl->running;
}

unsigned short CMcpServer::GetPort() const
{
    return m_pImpl->port;
}

// Global instance
CMcpServer g_McpServer;
