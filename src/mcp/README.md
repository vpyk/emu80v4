# BKEmu MCP server

This folder contains the (in-progress) **MCP (Model Context Protocol)** server
that lets an AI client (Claude Code, Claude Desktop, and other MCP-capable
assistants) attach to a **running** BKEmu instance and drive debugging and
reverse-engineering tasks:

- examine / modify CPU registers, PSW and ports;
- examine / modify memory; disassemble and assemble;
- set / remove execution and memory-access breakpoints;
- run / stop / reset the CPU and single-step (into / over / out);
- attach / detach disk images;
- save / load emulator states.

The idea: the user keeps BKEmu open as usual; the AI connects to it over a local
network port and performs debugging on the live machine, the same way a human
would through the debugger panes.

---

## 1. Why MCP, and how it connects

MCP is a small JSON-RPC 2.0 protocol that AI assistants speak to call external
"tools". Each capability above becomes one MCP *tool* the assistant can invoke.

**Transport: Streamable HTTP bound to `127.0.0.1` only.** The server runs as a
background thread inside the GUI app for the whole lifetime of the application;
the AI client connects to the already-running emulator.

We deliberately do **not** use the more common "stdio" transport, because stdio
requires the *client* to launch the server process. BKEmu is a long-running GUI
the user already has open, so an embedded HTTP server that the client *attaches*
to is the natural fit.

Security: the listener is bound to loopback only (never a routable interface —
this is a debugger with full memory write access). An optional bearer token will
gate requests. The server will default to **disabled** and be switched on from
the settings.

A representative client setup (Claude Code):

    claude mcp add --transport http bkemu http://127.0.0.1:19266/mcp

---

## 2. Architecture

Everything the server needs already exists as methods on the emulator's core
objects; the MCP layer is a thin protocol + threading shim over them:

- `CPlatform g_Platform` — run/stop/break/step, reset, memory access, disk
  attach/detach, save-state load/save, and `GetDebuggerPtr()`.
- `CDebugger` (via `g_Platform.GetDebuggerPtr()`) — registers, ports, memory
  dump get/set, breakpoints, disassembler and assembler.
- `CMainFrame` — the MFC main window; owns the UI thread and the message pump.

### Threading model and why marshalling is required

BKEmu already runs on two threads:

1. the **MFC main (UI) thread** — message pump, debugger panes, menu handlers;
2. the **emulation timer thread** (`CPlatform::TimerThreadFunc` → `FrameParam`),
   which runs the CPU frame-by-frame.

The MCP HTTP server adds worker threads (one per request, from cpp-httplib's
pool). The debugger get/set functions are written to be called from the UI
thread while the CPU is in a debug break, and several of them post/send window
messages (e.g. `CPlatform::BreakCPU` posts `WM_CPU_DEBUGBREAK`). Calling them
directly from an HTTP worker thread would be unsafe.

**Therefore every MCP operation is marshalled onto the UI thread** via a custom
window message, `WM_MCP_EXEC`, and the worker thread blocks until the UI thread
has executed it:

    HTTP worker thread                       MFC main (UI) thread
    ------------------                       --------------------
    parse JSON-RPC request
    build a std::function<void()>
    mcp::Run(fn):
      PostMessage(WM_MCP_EXEC, &cmd)  ─────►  CMainFrame::OnMcpExec(lParam)
      WaitForSingleObject(done) ........        mcp::Dispatch(cmd):
                                                  cmd.fn()   // safe: g_Platform etc.
                                        ◄─────      SetEvent(done)
    (fn has run; read its results)
    serialize JSON-RPC response

This gives each future tool handler one simple guarantee: *its lambda runs on
the UI thread*, so it may call any `g_Platform` / `CDebugger` / `CMainFrame`
method exactly as the existing menu handlers do.

---

## 3. Implementation status

| Milestone | Description | Status |
|-----------|-------------|--------|
| 1 | Scaffolding + vendored third-party headers | **done** |
| 2 | `WM_MCP_EXEC` UI-thread marshalling glue | **done** |
| 3 | `McpServer` + JSON-RPC handshake + first read-only tool (`cpu_state`) | **done** |
| 4 | Read-only debugging tools (memory, disasm, registers, ports, bp list) | planned |
| 5 | Mutating tools (register/memory/port write, breakpoints, step/run/stop) | planned |
| 6 | Media & state (disk attach/detach, save/load state) | planned |
| 7 | Hardening (token auth, settings UI, timeouts, error reporting) | planned |

Everything committed so far compiles and links cleanly into `BK_x64.exe`
(Debug x64, MSVC v143, `stdcpp20`) with **0 errors**. There is no caller of the
marshalling glue yet (the server arrives in milestone 3), so at runtime the
existing application behaviour is **unchanged**.

### Milestone 1 — scaffolding + headers (done)

Two header-only libraries vendored under `mcp/3rdparty/` so the build needs no
package manager:

| Library       | Version | Purpose | Source |
|---------------|---------|---------|--------|
| nlohmann/json | 3.11.3  | JSON parse/serialize for JSON-RPC | https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp |
| cpp-httplib   | 0.18.3  | embedded HTTP server (loopback)   | https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.3/httplib.h |

Both are MIT-licensed. To bump a version, re-download from the same URL pattern
with the new tag and update this table.

### Milestone 2 — UI-thread marshalling glue (done)

New files:

- `mcp/McpMarshal.h` — declares `WM_MCP_EXEC` (`WM_USER + 140`, above the
  `WM_USER+1xx` range already used in `Config.h`), the `mcp::McpCommand` struct,
  and the API `SetTargetWindow` / `IsReady` / `Run` / `Dispatch`.
- `mcp/McpMarshal.cpp` — the implementation.

The marshalling layer is intentionally JSON-agnostic: `mcp::Run` takes a plain
`std::function<void()>` that captures its own inputs and outputs. JSON only
enters the picture in the server/tools layer (milestone 3+).

Two deliberate safety choices, both documented in the source:

- **The wait in `mcp::Run` is unbounded.** `McpCommand` lives on the worker's
  stack and the lambda captures by reference, so the worker must not return
  until the UI thread has finished running it — otherwise the captures would
  dangle. A cancellable, bounded wait can be layered on later if a wedged UI
  ever proves to be a problem.
- **Self-call protection.** If `mcp::Run` is ever invoked on the UI thread, it
  runs the lambda inline instead of posting to itself and dead-locking.

### Milestone 3 — HTTP server, JSON-RPC handshake, first tool (done)

New files:

- `mcp/McpServer.h` — `CMcpServer` (PIMPL): `Start(port)` / `Stop()` /
  `IsRunning()` / `GetPort()`, plus the application-wide `g_McpServer` instance.
  The header pulls in no httplib / json / `<windows.h>`, so it is safe to
  include from MFC sources.
- `mcp/McpServer.cpp` — the cpp-httplib listener, the JSON-RPC 2.0 dispatcher,
  the MCP method handlers, and the tool registry.

What it implements:

- A single HTTP endpoint `POST /mcp` carrying JSON-RPC 2.0 (single message or a
  batch array). `GET /mcp` returns `405` (we do not offer a server-initiated SSE
  stream — the model only ever calls us). Notifications (messages without an
  `id`) get an empty `202` response.
- MCP methods: `initialize` (capabilities/serverInfo, echoes the client's
  requested `protocolVersion`), `tools/list`, `tools/call`, and `ping`. Unknown
  methods return JSON-RPC error `-32601`; parse failures `-32700`.
- A small tool registry (see the **Tools** section at the end of this document).
  Every tool does its emulator access inside `mcp::Run`, so it is marshalled onto
  the UI thread.
- Tool *execution* errors are returned as a normal result with `isError: true`
  and a human-readable message (so the model can see and react), rather than as
  a transport-level JSON-RPC error.

Lifecycle: the server is started from `CMainFrame::OnCreate`
(`g_McpServer.Start(19266)`) and stopped at the very start of `CMainFrame::OnClose`,
before the marshalling target window is cleared. (Port/enable will move to the
settings in milestone 7; for now it auto-starts on `127.0.0.1:19266`.)

#### Verified end-to-end

Built Debug x64 (0 errors) and driven with `curl`. Protocol and lifecycle:

- `initialize` → correct capabilities / serverInfo, negotiated protocol version;
- `tools/list` → advertises the registered tools with their input schemas;
- `ping` → `{}`; notification → `202`; unknown method → `-32601`; `GET` → `405`;
- closing the main window shuts the server down cleanly (port released, no hang).

Per-tool verification is recorded with each tool in the **Tools** section.

#### Trying it

With BKEmu running, from any MCP-capable client, e.g. Claude Code:

    claude mcp add --transport http bkemu http://127.0.0.1:19266/mcp

or a raw smoke test:

    curl -s -X POST http://127.0.0.1:19266/mcp -H "Content-Type: application/json" \
      -d "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"tools/call\",\"params\":{\"name\":\"cpu_state\",\"arguments\":{}}}"

### Changes to existing files

Kept minimal and localised; all additive.

- `MainFrm.h`
  - declared `afx_msg LRESULT OnMcpExec(WPARAM, LPARAM);` alongside the other
    `WM_USER` message handlers;
  - moved `LoadMemoryState(const fs::path&)` from `protected` to `public` so the
    `state_load` tool can reuse the full load orchestration (stop / load /
    reattach / reinit / run). No behaviour change.
  - moved `UpdateToolbarDriveIcons()` from `protected` to `public` so the
    `disk_attach`/`disk_detach` tools can refresh the drive icons. No behaviour
    change.
- `MainFrm.cpp`
  - `#include "mcp/McpMarshal.h"` and `#include "mcp/McpServer.h"`;
  - `ON_MESSAGE(WM_MCP_EXEC, &CMainFrame::OnMcpExec)` in the message map;
  - `OnMcpExec` implementation (delegates to `mcp::Dispatch`);
  - in `OnCreate`: `mcp::SetTargetWindow(GetSafeHwnd())` then
    `g_McpServer.Start(19266)`;
  - in `OnClose`: `g_McpServer.Stop()` then `mcp::SetTargetWindow(nullptr)`,
    before the existing shutdown.
- `BK.vcxproj` / `BK.vcxproj.filters`
  - added the `mcp\*.h` / `mcp\*.cpp` files and the two vendored headers under a
    new `mcp` (and `mcp\3rdparty`) filter group;
  - `mcp\McpServer.cpp` is marked `<PrecompiledHeader>NotUsing</PrecompiledHeader>`
    so it can control its own include order (see build notes below).

---

## 4. Layout

    mcp/
      README.md            this file
      McpMarshal.h         WM_MCP_EXEC message + UI-thread marshalling API
      McpMarshal.cpp       marshalling implementation
      McpServer.h          CMcpServer (PIMPL) + g_McpServer
      McpServer.cpp        HTTP listener, JSON-RPC dispatch, tool registry
      3rdparty/
        json.hpp           nlohmann/json  v3.11.3  (single-header, MIT)
        httplib.h          cpp-httplib    v0.18.3  (single-header, MIT)

---

## 5. Build / integration notes

- **Include paths are intentionally *not* modified.** MCP files are referenced
  by their folder-qualified path from project-root sources, e.g.
  `#include "mcp/McpMarshal.h"` (resolved via the existing `$(ProjectDir)` entry
  in `AdditionalIncludeDirectories`). Sources inside `mcp/` include their
  siblings directly (e.g. `#include "McpServer.h"`) and the vendored headers as
  `"3rdparty/json.hpp"` / `"3rdparty/httplib.h"`.
- **Winsock include order.** `httplib.h` requires `<winsock2.h>` *before*
  `<windows.h>`, and both it and `json.hpp` use `std::min` / `std::max` which the
  `<windows.h>` `min`/`max` macros would break. MFC's precompiled header drags in
  `<windows.h>` first, and `/Yu` ignores any text before the PCH include — so
  `McpServer.cpp` is compiled with **the PCH disabled** and includes
  `httplib.h` + `json.hpp` first, then `pch.h` and the emulator headers. This is
  the only file that touches the heavy `httplib.h`, keeping its compile-time cost
  contained.
- `httplib.h` pulls in Winsock and self-links `ws2_32.lib` via
  `#pragma comment(lib, ...)` on MSVC, so no manual linker change is needed
  (confirmed: links with 0 errors).

---

## 6. Tools

This is the growth point of the project — each new capability is added here as a
tool. All tools report numbers in octal (BK convention) and, where useful, also
in decimal. Where a tool takes an address/number, it accepts an octal string by
default, a `0x`-prefixed hex string, or a decimal JSON integer (masked to 16
bits). Every tool runs its emulator access on the UI thread via `mcp::Run`.

Most tools return their result as a JSON text block; a tool may instead return
richer MCP content (e.g. `screen` returns an image block the model can see) by
emitting a `_content` array, which the server passes through verbatim.

Each entry below notes what it does and how it was verified end-to-end (Debug
x64, driven with `curl`).

### `cpu_state` (read-only)

Registers `r0`-`r5`, `SP`, `PC`, `PSW` with decoded flags (C/V/Z/N/T/P),
run/break status, and the last executed instruction.

*Verified:* live register values returned, marshalled from the UI thread while
the CPU was running.

### `regs_set` (registers)

Set any subset of `r0`-`r5`, `sp`, `pc`, `psw` in a single call. The argument
object's keys are register names; each value is an octal string by default, a
`0x`-hex string, or a decimal integer. Requires the CPU to be paused (matches the
GUI register editor). Unknown keys are rejected, and at least one register must
be given. PSW read-only bits (8-9) are ignored by the hardware. Returns the
effective value of each register set (read back after writing).

*Verified:* set five registers at once with mixed formats — `r0="1000"` (octal),
`r1="0x20"` (hex), `sp=1234` (decimal), `pc="2000"`, `psw="340"` — and confirmed
via `cpu_state`; setting PSW to `177777` yields effective `176377` (read-only
bits preserved). Setting while running, an unknown register, an empty request,
and a bad value each return an `isError`.

### `sys_info` (read-only)

Orientation info for an agent:

- `mcp`: server name, version, protocol version, listening port;
- `machine`: config id (e.g. `BK-0010-01`) and localized human name, model
  number, board / MPI-device model codes, `is_bk10/11/11m`, whether a board is
  present;
- `cpu`: current / nominal / min / max clock in Hz, an `overclocked` flag, and
  run/break status;
- `peripherals`: booleans for FDD, HDD, AltPro, OPTOK, IRPS, AZBK and BK11M mode,
  decoded from the platform status flags.

*Verified:* correct machine/config (incl. the localized name `БК 0010-01`
round-tripped as UTF-8), CPU clock and peripheral flags.

### `disasm` (read-only)

Disassemble a run of instructions.

- `addr` — start address; if omitted, starts at the current PC.
- `count` — number of instructions (default 16, max 64).

Each entry has the address, raw words and plain disassembly text (the debugger's
syntax-highlight tags are stripped), plus `start` and `next` (the address
following the run). Reads memory with no bus side effects.

*Verified:* correct disassembly from the current PC and from an explicit address
(e.g. ROM at `100000` → `JMP 100260`); bad input reported as an `isError` result.

### `mem_read` (read-only)

Read a range of memory without bus side effects.

- `addr` — start address (required).
- `len` — number of bytes (decimal, default 64, max 4096).

Returns the start address and `end` (the address just past the range), both
octal, the byte count, a compact lowercase `hex` string (two digits per byte, no
separators), and an `ascii` rendering (non-printable bytes shown as `.`).

Byte values are hex (the universal, compact dump format) even though addresses
and word-level values elsewhere are octal; this keeps dumps small over the wire
and easy for a model to index.

*Verified:* bytes cross-checked against the disassembler — reading `100000`
returned the little-endian bytes of the known words `000167 000254 100742`
(`hex 7700ac00e281`, `ascii "w....."`); `0x`-hex and octal addresses resolve to
the same location; `len` is clamped to 4096; a missing `addr` is an `isError`.

### `screen` (read-only)

Capture the current display as a PNG, returned as an MCP **image content block**
so the model can actually *see* the BK screen (plus a text block of metadata).

- `size` — `native` (raw texture: 512×256 BK / 1024×768 AZBK; the default) or
  `view` (stretched to the current aspect-corrected viewport).

The screen is captured exactly as shown now (current colour/BW/adaptive mode and
palette); those, plus `width`/`height`/`bytes`, are reported in the metadata.
There is deliberately **no** colour-mode parameter: forcing a mode would mean
switching the live display, re-rendering and switching back (a visible flicker),
so the tool only ever reads what is currently on screen. Implementation reuses
the existing screenshot path (`CScreen::GetScreenshot` → `CImage` → GDI+ PNG into
an in-memory stream), encoded to base64.

*Verified:* returns a valid PNG (signature + `IHDR` checked) — `native` 512×256
(~2.5 KB), `view` 768×576; the decoded image is the real BK BASIC boot screen;
an invalid `size` is an `isError`.

### `bp_list` (read-only)

List all breakpoints. Each entry: `index`, `addr` (octal), `type` (`exec` or
`access`), `enabled`, `breaked` (currently triggered), and for access
breakpoints an `access` object (`read`/`write`/`byte`).

*Verified:* reflects adds/removes/clears correctly throughout the lifecycle test.

### `bp_set` (breakpoints)

Add a breakpoint.

- `addr` — required.
- `type` — `exec` (break when PC reaches the address; the default) or `access`
  (break on memory access).
- `access` — for `access` type, a string of letters `r`/`w`/`b`: read, write,
  and `b` = also catch byte-sized accesses (without `b`, only word accesses
  trigger). Default `rwb`.

Adding a duplicate of the same type+address is a no-op (`added: false` with a
note); the result reports the resulting total `count`. On success the GUI
breakpoint pane is refreshed.

*Verified:* exec and access breakpoints added; duplicate returns `added: false`;
access flags decoded correctly (`rw` → byte off, default → `rwb`); addresses
normalized to octal.

### `bp_remove` (breakpoints)

Remove **every** breakpoint (exec and access) at an address; returns the number
removed and the remaining count. Implemented by erasing from the list directly,
because the debugger's own `RemoveBreakpoint` only handles exec breakpoints.

*Verified:* removes an exec breakpoint; also removes an access breakpoint (which
the stock API cannot); removing a missing address returns `removed: 0`.

### `bp_clear` (breakpoints)

Remove all breakpoints; returns how many were `cleared`.

*Verified:* clears the full list back to `count: 0`.

### `state_save` (state)

Save a full emulator state image (MSF) to a file.

- `path` — destination file path (required); created or overwritten.

The CPU is briefly paused for a consistent snapshot and restored afterwards.
Returns the path and file size in bytes. The destination directory is validated
up front so a bad path returns a structured error instead of popping the GUI's
modal "file open error" box (`SaveMemoryState` opens non-silently).

*Verified:* writes a ~45 KB `.msf`; a non-existent directory returns
`destination directory does not exist` (no message box); a missing `path` is an
`isError`.

### `state_load` (state)

Load a full emulator state image (MSF) from a file.

- `path` — existing state file (required).

The file is validated **silently** first (header type `MSF_STATE_ID`, version in
the supported range), so a bad/old/corrupt file is reported as an error rather
than popping a GUI message box. Loading then reconfigures the platform to the
saved machine and resumes it (reusing the GUI's `CMainFrame::LoadMemoryState`,
which was made public for this). Returns the loaded machine's config id/name, the
file version, and run state.

*Verified:* round-trip — saved at PC `101020`, ran on to `101120`, loaded, and
the PC returned to the saved `101020` region; reports config `БК 0010-01`,
version 23. Missing file → `file not found`; a non-MSF file → `cannot open state
file` (silently, no box); missing `path` → `isError`.

Paths are decoded from UTF-8 to a wide `fs::path` so non-ASCII (e.g. Cyrillic)
directories work correctly.

### `disk_attach` (disks)

Mount a floppy image into a drive.

- `drive` — drive letter `A`-`D` (required).
- `path` — image file path (required).

The mount is live (no CPU stop), as in the GUI, and the toolbar drive icons are
refreshed. Returns the drive, path and whether it mounted read-only.
`AttachImage` reports success whenever the *drive* exists even if the file could
not be opened, so the tool verifies with `IsAttached` and rolls the config back
on failure.

*Verified* (in a `BK-0010-01_FDD` config): mounts a real `.IMG` into A and a
different image into B; `read_only` reported; mounting the *same* exclusively
opened image into a second drive correctly fails and rolls back; missing file →
`image file not found`; a bad drive letter, missing `path`, or a configuration
without a floppy controller each return an `isError`.

### `disk_detach` (disks)

Eject the image from a drive.

- `drive` — drive letter `A`-`D` (required).

Returns the drive and whether an image had been attached; refreshes the toolbar
icons.

*Verified:* detaches an attached drive (`was_attached: true`); detaching an empty
drive returns `was_attached: false`; a missing `drive` is an `isError`.

### `emu_run` (execution control)

Resume free-running execution: leave the debug break if the CPU is paused, or
start it if it was stopped. Returns the resulting `running` / `breaked` state, a
`changed` flag (false if it was already free-running), and the current `pc`.

*Verified:* resumes from a debug break (`changed: true`); a second call while
already running is a no-op (`changed: false`) and the PC has advanced, confirming
execution actually continued.

### `debug_break` (execution control)

Pause the CPU into the debugger at the current PC. Idempotent: if it is already
paused nothing changes. Returns the resulting `running` / `breaked` state, a
`changed` flag, the `pc` where it stopped, and `instruction` — the disassembled
instruction at that PC (same shape as a `disasm` item: `addr`, `words`, `text`,
`len_words`).

*Verified:* pauses execution (`changed: true`, PC frozen across repeated reads);
a second call is a no-op (`changed: false`); `instruction` matches the PC.

### `debug_step` (execution control)

Single-step the paused CPU. `mode` is one of:

- `into` — execute one instruction, entering calls/traps (the default);
- `over` — execute one instruction, running subroutine calls to completion;
- `out`  — run until the current subroutine returns;
- `skip` — advance PC past the current instruction *without* executing it.

Requires the CPU to be paused (see `debug_break`). For `into`/`over`/`out` the
CPU is briefly resumed and re-breaks itself on the emulation thread; the tool
waits for that off the UI thread (5 s timeout). Returns the resulting `running` /
`breaked` state, the new `pc`, the `instruction` now at the PC (when paused), and
`completed` — false only if an `into`/`over`/`out` step timed out while still
running.

*Verified:* `into` advances one instruction (and follows taken branches); `skip`
advances the PC past the current instruction without executing it; `over` steps
across a branch; `out` on code with no enclosing subroutine reports
`completed: false` and leaves the CPU running (matching the UI). Stepping while
running, and an invalid mode, both return an `isError` result.

### `emu_reset` (execution control)

Reset the board, equivalent to the reset button — a cold reset of the CPU
followed by a restart. Optional hardware modifiers (both default false):

- `su` — СУ/reset: restart at `100000` on BK11 (no effect on BK10);
- `long_reset` — long reset, relevant to the A16M controller.

Returns the post-reset `running`/`breaked` state and `pc`. Backed by
`CPlatform::ResetBoard`. (`ResetPlatform` is intentionally **not** exposed: it is
the handler for the CPU `RESET` instruction / init-time peripheral reset, not a
user-facing reset.)

*Verified:* from a deep PC, reset returns `pc: 100000` (the BK10 cold-start
vector) and the machine reboots (a follow-up break shows it running through the
boot ROM); `su`/`long_reset` are honoured/echoed; a non-boolean argument returns
an `isError`.
