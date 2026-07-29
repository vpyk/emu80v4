# Emu80 MCP server

Embedded **MCP (Model Context Protocol)** server that lets AI assistants (Claude
Code, Claude Desktop, etc.) attach to a **running** Emu80 instance and drive
debugging and reverse-engineering tasks for Soviet-era home computers.

## Transport

**Streamable HTTP on `127.0.0.1` only** (port `MCP_SERVER_PORT` = 19781,
defined in `McpServer.h`). Runs as a background thread inside the GUI app.
Security: loopback-only (full memory write access).

Client setup (Claude Code):

    claude mcp add --transport http emu80 http://127.0.0.1:19781/mcp

## Architecture

All emulator access is marshalled onto the main thread via `mcp::Run()` using
`std::mutex` + `std::condition_variable` (cross-platform C++17). Tools are thin
wrappers over existing emulator objects:

- `g_emulation` → `getCurrentPlatform()` — Platform access
- `Platform` — CPU, memory, keyboard, disks, renderer, debugger
- `Cpu8080Compatible` / `CpuZ80` — registers, stepping
- `ExternalDebugger` — breakpoints, run control
- `CrtRenderer` — screen capture, text screen

## Build

Qt 6 with MinGW (primary):

    qmake MCP_SERVER=1 src/Emu80qt.pro
    mingw32-make -j4

Third-party headers vendored under `3rdparty/`:
- `nlohmann/json` v3.11.3 (MIT)
- `cpp-httplib` v0.18.3 (MIT)
- `stb_image_write.h` v1.16 (public domain)

## Tools (24 total)

All numbers: JSON integer = decimal, JSON string = hex by default
(prefix `0x` = hex, `0o` = octal, leading `0` = decimal).

### System state

| Tool | Params | Description |
|------|--------|-------------|
| `cpu_state` | — | Registers (AF/BC/DE/HL/SP/PC), flags (S/Z/H/P/N/C), IFF, Z80 alt regs, instruction at PC, breaked/paused state, cpu_ticks since reset |
| `regs_set` | `af`/`bc`/`de`/`hl`/`sp`/`pc`/`psw`/`iff` (+ Z80: `af2`/`bc2`/`de2`/`hl2`/`ix`/`iy`/`i`/`r`/`im`) | Set registers (requires breaked) |
| `sys_info` | — | Server info, platform name, CPU type/frequency_hz, breaked/paused state, speed factor, available platforms |

### Execution control

| Tool | Params | Description |
|------|--------|-------------|
| `emu_run` | — | Resume execution (skips BP at current PC) |
| `emu_run_for` | `ms` (1–60000) | Run for N ms, stop on timer or breakpoint. Returns full cpu_state |
| `debug_break` | — | Pause CPU (also exits paused state). Idempotent |
| `debug_step` | `mode` (`into`/`over`/`skip`), `timeout_ms` | Single-step. `skip` advances PC without executing |
| `emu_reset` | — | Cold reset |
| `emu_load` | `file`, `autorun` (default true), `break_after_load` (default false) | Load file into emulator. If break_after_load: stop + cpu_state |
| `switch_platform` | `platform` | Switch to another platform (name from sys_info) |

### Memory & ports

| Tool | Params | Description |
|------|--------|-------------|
| `mem_read` | `addr`, `len` (default 64, max 4096) | Read memory range, hex dump + ASCII |
| `mem_write` | `addr`, `data` (hex string) | Write bytes to memory (max 4096) |
| `port_read` | `port` (0–255) | Read single byte from I/O port |
| `port_write` | `port` (0–255), `value` | Write single byte to I/O port |
| `disasm` | `addr` (default PC), `count` (default 16, max 64) | Disassemble instructions |

### Breakpoints

| Tool | Params | Description |
|------|--------|-------------|
| `bp_list` | — | List all BPs (exec, data, port) with hitCount/skipCount/remaining/comment |
| `bp_set` | `addr`, `type` (`exec`/`write`/`read`/`access`/`port_read`/`port_write`/`port_access`), `skipCount`, `comment` | Add or configure breakpoint. `exec` = code; `write`/`read`/`access` = memory; `port_*` = I/O ports |
| `bp_remove` | `addr`, `type` (optional filter) | Remove BPs at addr. With type: selective removal |
| `bp_clear` | — | Remove ALL breakpoints |

### Screen

| Tool | Params | Description |
|------|--------|-------------|
| `screen` | — | Capture display as PNG (MCP image content block) |
| `text_screen` | — | Return text screen (UTF-8). Only on text-mode platforms |

### Keyboard

| Tool | Params | Description |
|------|--------|-------------|
| `kbd_send` | `keys` (array), `action` (`press`/`release`/`tap`), `duration_ms` | Press/release/hold keys. `tap` blocks until done or breakpoint |
| `kbd_reset` | — | Release all virtually pressed keys. Also stops KbdTapper |
| `kbd_text` | `text` | Type a string via KbdTapper (platform-dependent). Blocks until done or breakpoint |

### Disks

| Tool | Params | Description |
|------|--------|-------------|
| `disk_attach` | `drive` (A–D or HDD), `file` | Mount disk image |
| `disk_detach` | `drive` (A–D or HDD) | Eject disk image |

## Threading

```
HTTP worker thread                    Main thread
------------------                    -----------
parse JSON-RPC request
build lambda capturing [&]
mcp::Run(fn):
  push command to queue
  wait on condition_variable ──────►  Emulation::mainLoopCycle()
                                       mcp::ProcessPendingCommands()
                                         cmd->fn()  // safe: Platform, CPU, etc.
                               ◄────    notify condition_variable
(fn has run; read results)
serialize JSON-RPC response
```

## Layout

    mcp/
      README.md
      McpMarshal.h         C++17 thread marshalling API
      McpMarshal.cpp       implementation
      McpServer.h          CMcpServer (PIMPL) + MCP_SERVER_PORT + g_McpServer
      McpServer.cpp        HTTP listener, JSON-RPC dispatch, all tools
      3rdparty/
        json.hpp           nlohmann/json v3.11.3
        httplib.h          cpp-httplib v0.18.3
        stb_image_write.h  stb v1.16 (PNG encoder for screen tool)
