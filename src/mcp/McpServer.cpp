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
#include "../Platform.h"
#include "../Debugger.h"
#include "../Cpu8080.h"
#include "../CpuZ80.h"
#include "../Cpu8080dasm.h"
#include "../CpuZ80dasm.h"
#include "../CrtRenderer.h"
#include "../AddrSpace.h"
#include "../EmuConfig.h"
#include "../Keyboard.h"
#include "../KbdLayout.h"
#include "../KbdTapper.h"

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
    uint16_t ParseNumArg(const json& v, const char* what)
    {
        try {
            if (v.is_number_integer())
                return static_cast<uint16_t>(v.get<long long>() & 0xFFFF);

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
                    return static_cast<uint16_t>(val & 0xFFFF);
            }
        } catch (...) {
        }

        throw std::runtime_error(std::string("invalid ") + what + " value");
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
    // Helper: fill json with CPU state (registers, flags, Z80 alt regs,
    // instruction at PC). Must be called on the main thread.
    // ================================================================
    void FillCpuStateJson(Cpu8080Compatible* cpu, json& state)
    {
        json regs;
        regs["af"] = RegVal(cpu->getAF());
        regs["bc"] = RegVal(cpu->getBC());
        regs["de"] = RegVal(cpu->getDE());
        regs["hl"] = RegVal(cpu->getHL());
        regs["sp"] = RegVal(cpu->getSP());
        regs["pc"] = RegVal(cpu->getPC());
        state["registers"] = regs;

        const uint16_t af = cpu->getAF();
        const uint16_t f  = af & 0xFF;
        state["psw"] = RegVal(f);
        state["flags"] = {
            { "S", !!(f & 0x80) }, { "Z", !!(f & 0x40) },
            { "H", !!(f & 0x10) }, { "P", !!(f & 0x04) },
            { "N", !!(f & 0x02) }, { "C", !!(f & 0x01) },
        };
        state["iff"] = cpu->getInte();

        CpuZ80* z80 = dynamic_cast<CpuZ80*>(cpu);
        if (z80) {
            state["cpu_type"] = "z80";
            json alt;
            alt["af2"] = RegVal(z80->getAF2());
            alt["bc2"] = RegVal(z80->getBC2());
            alt["de2"] = RegVal(z80->getDE2());
            alt["hl2"] = RegVal(z80->getHL2());
            alt["ix"]  = RegVal(z80->getIX());
            alt["iy"]  = RegVal(z80->getIY());
            alt["i"]   = RegVal(z80->getI());
            alt["r"]   = RegVal(z80->getR());
            alt["im"]  = z80->getIM();
            state["z80_regs"] = alt;
        } else {
            state["cpu_type"] = "8080";
        }

        // Current instruction at PC
        uint16_t pc = cpu->getPC();
        AddressableDevice* as = cpu->getAddrSpace();
        if (as) {
            uint8_t buf[4];
            buf[0] = as->readByte(pc);
            buf[1] = as->readByte(pc + 1);
            buf[2] = as->readByte(pc + 2);
            buf[3] = as->readByte(pc + 3);
            Cpu::CpuType ct = cpu->getType();
            if (ct == Cpu::CPU_8080)
                state["instruction"] = i8080GetInstructionMnemonic(buf);
            else {
                unsigned length; STEP_FLAG flag;
                state["instruction"] = cpu_disassemble_z80(pc, buf, length, flag);
            }
        }
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

            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
            if (!cpu) { err = "CPU is not available"; return; }

            state["breaked"]  = g_emulation->isDebuggerActive();
            state["paused"]   = g_emulation->getPausedState();
            FillCpuStateJson(cpu, state);
        });

        if (!ok) throw std::runtime_error("emulator main thread is not ready");
        if (!err.empty()) throw std::runtime_error(err);
        return state;
    }

    // ================================================================
    // Tool: regs_set
    // ================================================================
    struct RegDef { const char* name; bool isZ80; };

    json Tool_RegsSet(const json& args)
    {
        // Parse all keys upfront.
        std::vector<std::string>  names;
        std::vector<uint16_t>     values;
        for (auto it = args.begin(); it != args.end(); ++it) {
            std::string key = it.key();
            for (char& c : key)
                if (c >= 'A' && c <= 'Z')
                    c = static_cast<char>(c - 'A' + 'a');

            const char* valid[] = {
                "af", "bc", "de", "hl", "sp", "pc", "psw", "iff",
                "af2", "bc2", "de2", "hl2", "ix", "iy", "i", "r", "im"
            };
            bool found = false;
            for (const char* vn : valid) {
                if (key == vn) { found = true; break; }
            }
            if (!found)
                throw std::runtime_error("unknown register '" + it.key() + "' (use af, bc, de, hl, sp, pc, psw, iff, af2, bc2, de2, hl2, ix, iy, ir, im)");
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

            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
            if (!cpu) { err = "CPU is not available"; return; }

            if (!g_emulation->isDebuggerActive()) {
                err = "CPU is not paused; call debug_break first";
                return;
            }

            json regsOut;
            for (size_t i = 0; i < names.size(); ++i) {
                const std::string& n = names[i];
                uint16_t v = values[i];

                if (n == "af")      cpu->setAF(v);
                else if (n == "bc") cpu->setBC(v);
                else if (n == "de") cpu->setDE(v);
                else if (n == "hl") cpu->setHL(v);
                else if (n == "sp") cpu->setSP(v);
                else if (n == "pc") cpu->setPC(v);
                else if (n == "iff")cpu->setIFF(v != 0);
                else if (n == "psw") {
                    uint16_t af = cpu->getAF();
                    cpu->setAF((af & 0xFF00) | (v & 0xFF));
                }
                else {
                    CpuZ80* z80 = dynamic_cast<CpuZ80*>(cpu);
                    if (!z80) { err = "Z80 register not available on 8080"; continue; }
                    if (n == "af2")      z80->setAF2(v);
                    else if (n == "bc2") z80->setBC2(v);
                    else if (n == "de2") z80->setDE2(v);
                    else if (n == "hl2") z80->setHL2(v);
                    else if (n == "ix")  z80->setIX(v);
                    else if (n == "iy")  z80->setIY(v);
                    else if (n == "i")   z80->setI(static_cast<uint8_t>(v & 0xFF));
                    else if (n == "r")   z80->setR(static_cast<uint8_t>(v & 0x7F));
                    else if (n == "im")  z80->setIM(static_cast<uint8_t>(v & 0x3));
                }
            }

            // Read back
            json r;
            r["af"] = RegVal(cpu->getAF());
            r["bc"] = RegVal(cpu->getBC());
            r["de"] = RegVal(cpu->getDE());
            r["hl"] = RegVal(cpu->getHL());
            r["sp"] = RegVal(cpu->getSP());
            r["pc"] = RegVal(cpu->getPC());
            r["iff"] = cpu->getInte();
            CpuZ80* z80 = dynamic_cast<CpuZ80*>(cpu);
            if (z80) {
                r["af2"] = RegVal(z80->getAF2());
                r["ix"]  = RegVal(z80->getIX());
                r["iy"]  = RegVal(z80->getIY());
            }
            result["registers"] = r;
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
                Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
                json& cpuInfo = info["cpu"];
                cpuInfo["type"] = (cpu && dynamic_cast<CpuZ80*>(cpu)) ? "z80" : "8080";
                cpuInfo["clock"] = p->getCpuClock();
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
                    ap.push_back({ { "name", pi.platformName }, { "option", pi.cmdLineOption } });
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
            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
            if (!cpu) { err = "CPU is not available"; return; }
            AddressableDevice* as = cpu->getAddrSpace();
            if (!as) { err = "no address space"; return; }

            Cpu::CpuType ct = cpu->getType();
            uint16_t pc = hasAddr ? reqAddr : cpu->getPC();
            result["start"] = Hex(pc);

            json items = json::array();
            for (int n = 0; n < count; ++n) {
                uint8_t buf[4];
                buf[0] = as->readByte(pc);
                buf[1] = as->readByte(pc + 1);
                buf[2] = as->readByte(pc + 2);
                buf[3] = as->readByte(pc + 3);

                json item;
                item["addr"] = Hex(pc);

                unsigned len = 1;
                std::string text;
                if (ct == Cpu::CPU_8080) {
                    text = i8080GetInstructionMnemonic(buf);
                    len  = i8080GetInstructionLength(buf);
                } else {
                    STEP_FLAG flag;
                    text = cpu_disassemble_z80(pc, buf, len, flag);
                }
                item["text"]     = text;
                item["len_bytes"] = static_cast<int>(len);

                json bytesArr = json::array();
                for (unsigned i = 0; i < len && i < 4; ++i)
                    bytesArr.push_back(Hex(buf[i]));
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

    json Tool_MemRead(const json& args)
    {
        if (!args.contains("addr") || args["addr"].is_null())
            throw std::runtime_error("addr is required");

        const uint16_t addr = ParseNumArg(args["addr"], "addr");
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
            Cpu* cpu = p->getCpu();
            if (!cpu) { err = "CPU is not available"; return; }
            AddressableDevice* as = cpu->getAddrSpace();
            if (!as) { err = "no address space"; return; }

            static const char hexd[] = "0123456789abcdef";
            std::string hex, ascii;
            hex.reserve(static_cast<size_t>(len) * 2);
            ascii.reserve(static_cast<size_t>(len));

            for (int i = 0; i < len; ++i) {
                const uint8_t b = as->readByte(static_cast<uint16_t>(addr + i));
                hex.push_back(hexd[b >> 4]);
                hex.push_back(hexd[b & 0x0F]);
                ascii.push_back((b >= 0x20 && b < 0x7F) ? static_cast<char>(b) : '.');
            }

            result["addr"]  = Hex(addr);
            result["len"]   = len;
            result["end"]   = Hex(static_cast<uint16_t>(addr + len));
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

        const uint16_t addr = ParseNumArg(args["addr"], "addr");
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
            Cpu* cpu = p->getCpu();
            if (!cpu) { err = "CPU is not available"; return; }
            AddressableDevice* as = cpu->getAddrSpace();
            if (!as) { err = "no address space"; return; }

            int count = 0;
            for (size_t i = 0; i < clean.size(); i += 2) {
                uint8_t byteVal = static_cast<uint8_t>(
                    std::stoul(clean.substr(i, 2), nullptr, 16));
                as->writeByte(static_cast<uint16_t>(addr + count), byteVal);
                ++count;
            }

            result["addr"]  = Hex(addr);
            result["count"] = count;
            result["end"]   = Hex(static_cast<uint16_t>(addr + count));
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
            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
            if (!cpu) { err = "CPU is not available"; return; }

            // Use ExternalDebugger to skip breakpoint at current PC.
            IDebugger* dbg = p->getDebugger();
            if (!dbg) {
                p->createDebugger();
                dbg = p->getDebugger();
            }
            ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
            if (!extDbg) { err = "debugger not available in MCP mode"; return; }

            const bool wasBreaked = g_emulation->isDebuggerActive();
            extDbg->dbgRun();

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
            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
            if (!cpu) { err = "CPU is not available"; return; }

            IDebugger* dbg = p->getDebugger();
            if (!dbg) {
                p->createDebugger();
                dbg = p->getDebugger();
            }
            ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
            if (!extDbg) { err = "debugger not available in MCP mode"; return; }

            extDbg->dbgRunFor(static_cast<unsigned>(ms));
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
            Cpu8080Compatible* cpu = p ? dynamic_cast<Cpu8080Compatible*>(p->getCpu()) : nullptr;
            if (!cpu) { err = "CPU unavailable after run"; return; }

            IDebugger* dbg = p->getDebugger();
            ExternalDebugger* extDbg = dbg ? dynamic_cast<ExternalDebugger*>(dbg) : nullptr;
            bool timerDidFire = extDbg && extDbg->isTimerFired();
            if (extDbg)
                extDbg->dbgCleanupTimer();

            result["stoppedByTimer"] = timerDidFire;
            result["breaked"]        = g_emulation->isDebuggerActive();
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
            throw std::runtime_error("path (string) is required");

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
                Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
                if (cpu) {
                    g_emulation->debugRequest(dynamic_cast<Cpu*>(cpu));
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

            for (auto k : keys)
                kb->processKey(k, true);

            kbdTimer = new DebugElapsedTimer(cpu);
            kbdTimer->start(static_cast<unsigned>(durationMs));

            result["wasBreaked"] = g_emulation->isDebuggerActive();
            if (g_emulation->isDebuggerActive()) {
                IDebugger* dbg = p->getDebugger();
                if (!dbg) { p->createDebugger(); dbg = p->getDebugger(); }
                ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
                if (!extDbg) { err = "debugger not available in MCP mode"; return; }
                extDbg->dbgRun();
            }
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
                if (wasBreaked)
                    g_emulation->debugRequest(p->getCpu());
                else
                    g_emulation->debugRun();
            }

            result["breaked"]             = g_emulation->isDebuggerActive();
            result["stoppedByBreakpoint"] = stoppedByBreakpoint;
            result["completed"]           = !stoppedByBreakpoint;
            result["duration_ms"]         = durationMs;
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
                IDebugger* dbg = p->getDebugger();
                if (!dbg) { p->createDebugger(); dbg = p->getDebugger(); }
                ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
                if (!extDbg) { err = "debugger not available in MCP mode"; return; }
                extDbg->dbgRun();
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
                Cpu* cpu = p->getCpu();
                if (cpu)
                    g_emulation->debugRequest(cpu);
            }

            result["completed"]           = !stoppedByBreakpoint;
            result["stoppedByBreakpoint"] = stoppedByBreakpoint;
            result["breaked"]             = g_emulation->isDebuggerActive();
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
            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
            if (!cpu) { err = "CPU is not available"; return; }

            const bool wasBreaked = g_emulation->isDebuggerActive();
            if (!wasBreaked)
                g_emulation->debugRequest(dynamic_cast<Cpu*>(cpu));

            uint16_t pc = cpu->getPC();
            result["breaked"]  = g_emulation->isDebuggerActive();
            result["changed"]  = !wasBreaked;
            result["pc"]       = Hex(pc);

            // Disassemble instruction at PC
            AddressableDevice* as = cpu->getAddrSpace();
            if (as && g_emulation->isDebuggerActive()) {
                uint8_t buf[4];
                buf[0] = as->readByte(pc);
                buf[1] = as->readByte(pc + 1);
                buf[2] = as->readByte(pc + 2);
                buf[3] = as->readByte(pc + 3);
                if (cpu->getType() == Cpu::CPU_8080) {
                    result["instruction"] = i8080GetInstructionMnemonic(buf);
                } else {
                    unsigned length; STEP_FLAG flag;
                    result["instruction"] = cpu_disassemble_z80(pc, buf, length, flag);
                }
            }
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
        if (mode != "into" && mode != "over" && mode != "skip")
            throw std::runtime_error("invalid mode (expected into|over|skip)");

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
            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
            if (!cpu) { err = "CPU is not available"; return; }

            if (!g_emulation->isDebuggerActive()) {
                err = "CPU is not paused; call debug_break first";
                return;
            }

            // Get ExternalDebugger for proper breakpoint-aware stepping.
            IDebugger* dbg = p->getDebugger();
            if (!dbg) {
                p->createDebugger();
                dbg = p->getDebugger();
            }
            ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
            if (!extDbg) { err = "debugger not available in MCP mode"; return; }

            if (mode == "skip") {
                uint16_t pc = cpu->getPC();
                AddressableDevice* as = cpu->getAddrSpace();
                if (!as) { err = "no address space"; return; }
                uint8_t buf[4] = { as->readByte(pc), as->readByte(pc+1), as->readByte(pc+2), as->readByte(pc+3) };
                unsigned len = 1;
                if (cpu->getType() == Cpu::CPU_8080)
                    len = i8080GetInstructionLength(buf);
                else {
                    STEP_FLAG flag;
                    cpu_disassemble_z80(pc, buf, len, flag);
                }
                cpu->setPC(static_cast<uint16_t>(pc + len));
                return;
            }

            if (mode == "into") {
                extDbg->dbgStepIn();
            } else { // "over"
                extDbg->dbgStepOver();
            }

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
            Cpu8080Compatible* cpu = p ? dynamic_cast<Cpu8080Compatible*>(p->getCpu()) : nullptr;
            if (!cpu) { err = "CPU unavailable after step"; return; }

            uint16_t pc = cpu->getPC();
            result["breaked"]   = g_emulation->isDebuggerActive();
            result["completed"] = completed;
            result["pc"]        = Hex(pc);

            if (g_emulation->isDebuggerActive()) {
                AddressableDevice* as = cpu->getAddrSpace();
                if (as) {
                    uint8_t buf[4];
                    buf[0] = as->readByte(pc);
                    buf[1] = as->readByte(pc + 1);
                    buf[2] = as->readByte(pc + 2);
                    buf[3] = as->readByte(pc + 3);
                    if (cpu->getType() == Cpu::CPU_8080)
                        result["instruction"] = i8080GetInstructionMnemonic(buf);
                    else {
                        unsigned length; STEP_FLAG flag;
                        result["instruction"] = cpu_disassemble_z80(pc, buf, length, flag);
                    }
                }
            }
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
            Cpu8080Compatible* cpu = dynamic_cast<Cpu8080Compatible*>(p->getCpu());
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

            IDebugger* dbg = p->getDebugger();
            if (!dbg) {
                p->createDebugger();
                dbg = p->getDebugger();
            }

            ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
            if (!extDbg) { err = "debugger not available in MCP mode"; return; }

            DbgCpuState state;
            extDbg->dbgGetState(state);

            json items = json::array();
            int idx = 0;
            for (auto& bp : state.breakpoints) {
                items.push_back({
                    { "index",     idx++ },
                    { "addr",      Hex(bp.addr) },
                    { "type",      "exec" },
                    { "hitCount",  bp.hitCount },
                    { "skipCount", bp.skipCount },
                    { "remaining", bp.remaining },
                    { "comment",   bp.comment },
                });
            }
            for (auto& dbp : state.dataBreakpoints) {
                const char* typeStr = (dbp.type == BT_WRITE)  ? "write"
                                    : (dbp.type == BT_READ)   ? "read"
                                    : "access";
                items.push_back({
                    { "index",     idx++ },
                    { "addr",      Hex(dbp.addr) },
                    { "type",      typeStr },
                    { "hitCount",  dbp.hitCount },
                    { "skipCount", dbp.skipCount },
                    { "remaining", dbp.remaining },
                    { "comment",   dbp.comment },
                });
            }
            for (auto& dbp : state.portBreakpoints) {
                const char* typeStr = (dbp.type == BT_PORT_WRITE)  ? "port_write"
                                    : (dbp.type == BT_PORT_READ)   ? "port_read"
                                    : "port_access";
                items.push_back({
                    { "index",     idx++ },
                    { "addr",      Hex(dbp.addr) },
                    { "type",      typeStr },
                    { "hitCount",  dbp.hitCount },
                    { "skipCount", dbp.skipCount },
                    { "remaining", dbp.remaining },
                    { "comment",   dbp.comment },
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
        if (type != "exec" && type != "write" && type != "read" && type != "access"
            && type != "port_read" && type != "port_write" && type != "port_access")
            throw std::runtime_error("invalid type (expected exec|write|read|access|port_read|port_write|port_access)");

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

            IDebugger* dbg = p->getDebugger();
            if (!dbg) {
                p->createDebugger();
                dbg = p->getDebugger();
            }

            ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
            if (!extDbg) { err = "debugger not available in MCP mode"; return; }

            bool exists = false;
            if (type == "exec") {
                DbgCpuState state;
                extDbg->dbgGetState(state);
                for (auto& bp : state.breakpoints)
                    if (bp.addr == addr) { exists = true; break; }
                if (!exists) {
                    std::list<uint16_t> addrs;
                    addrs.push_back(addr);
                    extDbg->dbgSetBreakpoints(addrs);
                }
                if (skipCount >= 0)
                    extDbg->dbgSetExecSkipCount(addr, skipCount);
                if (hasComment)
                    extDbg->dbgSetExecComment(addr, comment);
            } else if (type == "port_read" || type == "port_write" || type == "port_access") {
                BreakpointType bt = (type == "port_write") ? BT_PORT_WRITE
                                 : (type == "port_read")  ? BT_PORT_READ
                                 : BT_PORT_ACSESS;
                DbgCpuState state;
                extDbg->dbgGetState(state);
                for (auto& dbp : state.portBreakpoints)
                    if (dbp.addr == addr && dbp.type == static_cast<int>(bt))
                        { exists = true; break; }
                if (!exists)
                    extDbg->dbgSetPortBreakpoint(addr, bt);
                if (skipCount >= 0)
                    extDbg->dbgSetPortSkipCount(addr, skipCount);
                if (hasComment)
                    extDbg->dbgSetPortComment(addr, comment);
            } else {
                BreakpointType bt = (type == "write") ? BT_WRITE
                                 : (type == "read")  ? BT_READ
                                 : BT_ACSESS;
                DbgCpuState state;
                extDbg->dbgGetState(state);
                for (auto& dbp : state.dataBreakpoints)
                    if (dbp.addr == addr && dbp.type == static_cast<int>(bt))
                        { exists = true; break; }
                if (!exists)
                    extDbg->dbgSetDataBreakpoint(addr, bt);
                if (skipCount >= 0)
                    extDbg->dbgSetDataSkipCount(addr, skipCount);
                if (hasComment)
                    extDbg->dbgSetDataComment(addr, comment);
            }

            result["added"] = !exists;
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
        if (!args.contains("addr") || args["addr"].is_null())
            throw std::runtime_error("addr is required");

        const uint16_t addr = ParseNumArg(args["addr"], "addr");

        // Optional type filter — if provided, only remove matching BPs.
        std::string type;
        bool hasType = args.contains("type") && args["type"].is_string();
        if (hasType) {
            type = args["type"].get<std::string>();
            if (type != "exec" && type != "write" && type != "read" && type != "access"
                && type != "port_read" && type != "port_write" && type != "port_access")
                throw std::runtime_error("invalid type (expected exec|write|read|access|port_read|port_write|port_access)");
        }

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            IDebugger* dbg = p->getDebugger();
            // No debugger means no breakpoints — return zero results.
            if (!dbg) {
                result["removed"] = 0;
                result["count"]   = 0;
                return;
            }

            ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
            if (!extDbg) {
                result["removed"] = 0;
                result["count"]   = 0;
                return;
            }

            DbgCpuState state;
            extDbg->dbgGetState(state);

            int removed = 0;

            // Remove exec breakpoints.
            if (!hasType || type == "exec") {
                std::list<uint16_t> toRemove;
                for (auto& bp : state.breakpoints) {
                    if (bp.addr == addr) { toRemove.push_back(bp.addr); ++removed; }
                }
                if (!toRemove.empty())
                    extDbg->dbgDelBreakpoints(toRemove);
            }

            // Remove data breakpoints.
            if (!hasType || type == "write" || type == "read" || type == "access") {
                for (auto& dbp : state.dataBreakpoints) {
                    if (dbp.addr != addr) continue;
                    if (hasType) {
                        BreakpointType bt = (type == "write") ? BT_WRITE
                                          : (type == "read")  ? BT_READ : BT_ACSESS;
                        if (dbp.type != static_cast<int>(bt)) continue;
                    }
                    extDbg->dbgDelDataBreakpoint(dbp.addr, static_cast<BreakpointType>(dbp.type));
                    ++removed;
                }
            }

            // Remove port breakpoints.
            if (!hasType || type == "port_read" || type == "port_write" || type == "port_access") {
                for (auto& dbp : state.portBreakpoints) {
                    if (dbp.addr != addr) continue;
                    if (hasType) {
                        BreakpointType bt = (type == "port_write") ? BT_PORT_WRITE
                                          : (type == "port_read")  ? BT_PORT_READ : BT_PORT_ACSESS;
                        if (dbp.type != static_cast<int>(bt)) continue;
                    }
                    extDbg->dbgDelPortBreakpoint(dbp.addr, static_cast<BreakpointType>(dbp.type));
                    ++removed;
                }
            }

            result["removed"] = removed;
            result["addr"]    = Hex(addr);
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

            IDebugger* dbg = p->getDebugger();
            // No debugger means no breakpoints — return zero results.
            if (!dbg) {
                result["removed"] = 0;
                result["count"]   = 0;
                return;
            }

            ExternalDebugger* extDbg = dynamic_cast<ExternalDebugger*>(dbg);
            if (!extDbg) {
                result["removed"] = 0;
                result["count"]   = 0;
                return;
            }

            DbgCpuState state;
            extDbg->dbgGetState(state);
            int execCount = static_cast<int>(state.breakpoints.size());
            int dataCount = static_cast<int>(state.dataBreakpoints.size());
            int portCount = static_cast<int>(state.portBreakpoints.size());

            if (execCount > 0) {
                std::list<uint16_t> addrs;
                for (auto& bp : state.breakpoints) addrs.push_back(bp.addr);
                extDbg->dbgDelBreakpoints(addrs);
            }
            if (dataCount > 0)
                extDbg->dbgClearDataBreakpoints();
            if (portCount > 0)
                extDbg->dbgClearPortBreakpoints();
            result["cleared"] = execCount + dataCount + portCount;
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
            throw std::runtime_error("drive (letter A-D) is required");
        std::string drive = args["drive"].get<std::string>();
        if (!drive.empty()) { drive[0] = static_cast<char>(std::toupper(drive[0])); }
        if (drive.size() != 1 || drive[0] < 'A' || drive[0] > 'D')
            throw std::runtime_error("invalid drive (expected A, B, C or D)");

        if (!args.contains("path") || !args["path"].is_string())
            throw std::runtime_error("path (string) is required");
        std::string path = args["path"].get<std::string>();

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            std::string propName = std::string("disk") + drive[0];
            p->setProperty(propName, {path});

            result["attached"] = true;
            result["drive"]    = drive;
            result["path"]     = path;
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
            throw std::runtime_error("drive (letter A-D) is required");
        std::string drive = args["drive"].get<std::string>();
        if (!drive.empty()) { drive[0] = static_cast<char>(std::toupper(drive[0])); }
        if (drive.size() != 1 || drive[0] < 'A' || drive[0] > 'D')
            throw std::runtime_error("invalid drive (expected A, B, C or D)");

        json        result;
        std::string err;

        const bool ok = mcp::Run([&]
        {
            Platform* p = GetPlatform();
            if (!p) { err = "no platform created"; return; }

            std::string propName = std::string("disk") + drive[0];
            p->setProperty(propName, {""});

            result["detached"] = true;
            result["drive"]    = drive;
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
            "provided for each register.",
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
                "(Z80 only). Number format: JSON integer = decimal, JSON string = hex "
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
            "'len' is byte count (default 64, max 4096). Returns compact hex dump "
            "and ASCII rendering (non-printable → '.').",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr", { { "type", json::array({ "string", "integer" }) }, { "description", "start address (required)" } } },
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
            "or \"C30001\"). Non-hex characters are stripped. Max 4096 bytes.",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr", { { "type", json::array({ "string", "integer" }) }, { "description", "start address (required)" } } },
                    { "data", { { "type", "string" }, { "description", "hex bytes to write (required)" } } },
                }},
                { "required", json::array({ "addr", "data" }) },
            },
            &Tool_MemWrite
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
            "Load a file into the emulated machine. 'path' is required. "
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
            "'over' (step over calls), or 'skip' (advance PC without executing). "
            "Optional 'timeout_ms' (1–60000, default 5000) limits wait for step "
            "completion. Requires CPU paused. Returns new PC and instruction.",
            json{
                { "type", "object" },
                { "properties", {
                    { "mode",       { { "type", "string" }, { "enum", json::array({ "into", "over", "skip" }) }, { "description", "step mode (default into)" } } },
                    { "timeout_ms", { { "type", "integer" }, { "minimum", 1 }, { "maximum", 60000 }, { "description", "timeout in ms (default 5000)" } } },
                }}
            },
            &Tool_DebugStep
        });

        tools.push_back({
            "emu_reset",
            "Reset the emulated machine (cold reset). Returns post-reset state "
            "and the PC at the reset vector.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_EmuReset
        });

        tools.push_back({
            "bp_list",
            "List all breakpoints with index, address (hex), and type.",
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
            "Optional 'skipCount' sets how many hits to skip before "
            "actually stopping (default 0 = stop immediately). "
            "Duplicates of the same type+addr are a no-op (but skipCount "
            "is still applied if provided).",
            json{
                { "type", "object" },
                { "properties", {
                    { "addr",     { { "type", json::array({ "string", "integer" }) }, { "description", "breakpoint address (required)" } } },
                    { "type",     { { "type", "string" }, { "enum", json::array({ "exec", "write", "read", "access", "port_read", "port_write", "port_access" }) }, { "description", "breakpoint type (default exec)" } } },
                    { "skipCount", { { "type", "integer" }, { "minimum", 0 }, { "description", "skip N hits before stopping (default 0)" } } },
                    { "comment",   { { "type", "string" }, { "description", "optional comment stored with the breakpoint" } } },
                }},
                { "required", json::array({ "addr" }) },
            },
            &Tool_BpSet
        });

        tools.push_back({
            "bp_remove",
            "Remove breakpoints at an address. 'addr' format: "
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
                }},
                { "required", json::array({ "addr" }) },
            },
            &Tool_BpRemove
        });

        tools.push_back({
            "bp_clear",
            "Remove ALL breakpoints. Returns the number that were cleared.",
            json{ { "type", "object" }, { "properties", json::object() } },
            &Tool_BpClear
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
            "Mount a disk image into drive A-D. 'drive' (A-D) and 'path' are "
            "required. Note: this sets the platform property; the emulator may "
            "need a reset for it to take effect on some platforms.",
            json{
                { "type", "object" },
                { "properties", {
                    { "drive", { { "type", "string" }, { "description", "drive letter A-D (required)" } } },
                    { "path",  { { "type", "string" }, { "description", "image file path (required)" } } },
                }},
                { "required", json::array({ "drive", "path" }) },
            },
            &Tool_DiskAttach
        });

        tools.push_back({
            "disk_detach",
            "Eject the disk from drive A-D. 'drive' (A-D) is required.",
            json{
                { "type", "object" },
                { "properties", {
                    { "drive", { { "type", "string" }, { "description", "drive letter A-D (required)" } } },
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
