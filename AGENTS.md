# AGENTS.md

This file provides guidance to AI agents when working with code in this repository.

## What this is

Emu80 v4 — a C++ emulator of Soviet/Eastern-bloc 8080/Z80 home computers (Radio-86RK, Apogey, Partner, Specialist, Orion, Vector, Korvet, ZX Spectrum, …). GPL-3.0. README, comments, config files and `whatsnew.txt` are mostly in Russian. There is no test suite and no linter; verification means building and running the emulator.

## Build

One source tree, four front-ends selected by a `PAL_*` define (see "Platform Abstraction Layer" below). All commands run from the repo root.

| Variant | Build | Output |
|---|---|---|
| Qt (primary) | `qmake src/Emu80qt.pro && make` (`qmake6`/`qmake-qt5` on some distros; `mingw32-make` on Windows) | `Emu80qt` |
| Qt + MCP server | `qmake MCP_SERVER=1 src/Emu80qt.pro && make` (or env var `MCP_SERVER=1`) | `Emu80qt-mcp` |
| SDL + wx | `make -f Makefile.sdlwx` | `Emu80` |
| Lite (SDL, no UI) | `make -f Makefile.lite` | `Emu80lite` |
| WebAssembly | `make -f Makefile.wasm` (needs Emscripten) | `build_wasm/` |
| WASM + debugger | `make -f Makefile.wasm_dbg` (adds `-DWASM_DBG`) | `build_wasm/` |

- `make install [-f Makefile.xxx]` does a portable install to `~/emu80` (binary + `dist/*` + docs). The emulator needs the `dist/` files (`emu80.conf`, platform dirs, shaders) next to the executable to run at all.
- Qt build intermediates go to `src/build/` (`obj`, `moc`, `ui`, `qrc`).
- **Language standard differs per target**: Qt is `c++17`, but the SDL/wx/Lite/WASM Makefiles compile with `-std=c++11`. Core code in `src/*.cpp` (shared by all variants) must stay C++11-compatible; C++17 is only safe in `src/qt/` and `src/mcp/`.
- **Adding a source file**: `Makefile.sdlwx`/`.lite`/`.wasm*` glob `src/*.cpp`, so nothing to do there, but `src/Emu80qt.pro` (SOURCES/HEADERS) and the Code::Blocks projects `src/Emu80.cbp`/`src/Emu80lnx.cbp` list files explicitly and must be updated by hand.
- Run with command-line options such as `--platform <name>`, `--conf-file`, `--run <file>`, `--load <file>`, `--disk-a <image>` (see `displayCmdLineHelp()` in `src/Main.cpp`). The start platform comes from `emu80.run`, else the platform chooser.

## Architecture

### Configuration-driven object graph (the key idea)

A "platform" (a specific computer) is not hard-coded: it is assembled at runtime from a `.conf` file in `dist/<platform>/`. `Platform`'s constructor (`src/Platform.cpp`) feeds the file to `ConfigReader`, which parses lines like

```
Ram ram = 0xEC00                 # <ClassName> <objName> = <ctor params>
Cpu8080 cpu
cpu.frequency = @CPU_FREQUENCY   # <obj>.<property> = <values>; @VAR are config variables
cpu.addrSpace = &addrSpace       # &name is a reference to another object
```

and instantiates objects via `ObjectFactory` (`src/ObjectFactory.cpp`, one `REG_EMU_CLASS(Class)` per class, each exposing `static EmuObject* create(const EmuValuesList&)`). Conf files support `include`, `ifdef/ifndef/else/endif` and variables. After parsing, `Platform` scans its object list with `dynamic_cast` to pick out the singleton roles: `EmuWindow`, `Cpu`, `PlatformCore`, `KbdLayout`, `CrtRenderer` (up to two), `DiskImage` (by label A–D/HDD), `FileLoader`, `Keyboard`, `RamDisk`, `KbdTapper`.

Consequences when changing things:
- **New device/class**: implement it as an `EmuObject` subclass, override `setProperty()`/`getPropertyStringValue()` for its config properties, add `create()`, and register it with `REG_EMU_CLASS` in `ObjectFactory.cpp`. Only then can `.conf` files use it.
- **New computer/variant**: mostly a new conf file plus an entry in `dist/emu80.conf` (`config.addPlatform = name, conf file, object name[, cmdline opt]`). Machine-specific C++ (`Rk86.cpp`, `Apogey.cpp`, `Orion.cpp`, `Zx.cpp`, …) supplies only the parts that generic modules can't (core, renderer, keyboard circuit, memory pagers).
- `.conf` files are UTF-8 **with BOM** and contain Cyrillic; preserve encoding when editing.
- `dist/<platform>/*.opt` and `emu80.opt` hold user-saved options and are `include`d by the confs; `ConfigTab`/`Config*Selector` objects in confs define the settings-dialog UI declaratively.

### Object model (`src/EmuObjects.h`)

- `EmuObject`: base of everything; has named **outputs** and **inputs** (`REG_OUTPUT`/`REG_INPUT`, connected in conf via `connect obj.output -> obj.input`) for signal wiring between devices, plus the property get/set interface used by the config reader and settings UI.
- `AddressableDevice` (`readByte`/`writeByte`): memory/IO-mapped things; composed via `AddrSpace` (range mapping), `AddrSpaceMapper`, etc.
- `ActiveDevice`/`IActive`: things that consume time (CPU, timers, CRT, DMA…). `Emulation` schedules all registered active devices against a shared clock (`Emulation::exec`), driven each frame from `Emulation::mainLoopCycle()`.
- `Emulation` (global `g_emulation`) owns the `EmuConfig` (platform list from `dist/emu80.conf`), the `SoundMixer`, and the list of running `Platform`s; `Platform` owns its objects and is a `ParentObject`.
- CPUs: `Cpu8080` (+`Cpu8080dasm`) and `CpuZ80` (+`CpuZ80dasm`), both deriving from `Cpu`. Z80 core is vendored in `src/3rdparty/z80/` (redcode/Z80). `CpuHook` subclasses (`RkTapeHooks`, `MsxTapeHooks`, …) intercept ROM addresses to redirect tape I/O to files.

### Platform Abstraction Layer (PAL)

The core (`src/*.cpp`) never touches a GUI/OS API directly; it calls `pal*` functions declared through `src/Pal.h`, which includes exactly one backend chosen by define:
- `PAL_QT` → `src/qt/` (Qt GUI, `.ui` forms, translations `emu80_ru.ts`, debugger window)
- `PAL_SDL` + `PAL_WX` → `src/sdl/` + `src/wx/`
- `PAL_SDL` + `PAL_LITE` → `src/sdl/` + `src/lite/` (no UI)
- `PAL_SDL` + `PAL_LITE` + `PAL_WASM` → additionally `src/wasm/` (browser glue in `src/wasm/web/`, `prejs.js`); WASM excludes `sdl/sdlPalFile.cpp`.

`PalWindow`/`PalFile` (`PalWindow.h`, `PalFile.h`) do the same dispatch for windows and file I/O. `EmuCalls.h`/`DbgCalls.h` are the reverse direction: entry points the PAL calls into the core. When adding a `pal*` function, implement it in every backend.

### Debugger and MCP server

`src/Debugger.{h,cpp}` defines `IDebugger`. Normal builds use the GUI `DebugWindow`; builds with `MCP_SERVER` or `WASM_DBG` instead use `ExternalDebugger` (see `Platform::createDebugger()`), i.e. **the GUI debugger is not available in the MCP build**.

`src/mcp/` is an embedded MCP (Model Context Protocol) server, Qt builds only, compiled with `-DMCP_SERVER`. It listens on `127.0.0.1:19781` in a background HTTP thread (vendored `cpp-httplib` + `nlohmann/json` in `src/mcp/3rdparty/`). Tool handlers must not touch emulator state directly: they wrap work in `mcp::Run(fn)` (`McpMarshal.cpp`), which queues it and blocks until the main thread executes it in `Emulation::mainLoopCycle()` → `mcp::ProcessPendingCommands()`. All tools are in `McpServer.cpp`; the tool list is documented in `src/mcp/README.md` and `doc/MCP-server.md` — update both when adding/changing a tool. Register with Claude Code via `claude mcp add --transport http emu80 http://127.0.0.1:19781/mcp`.

## Conventions

- Source files carry the GPL header with `© Viktor Pykhonin <pyk@mail.ru>, 2016-<year>`; keep it on new files. Comments are a mix of Russian and English.
- Release notes go in `whatsnew.txt` (top section for the current build; markers `+` new, `*` changed, `-` fixed, `!` known issue). The build number is `VER_BUILD` in `src/Version.h`.
- User docs live in `doc/` (`emu80_manual.md`, `emu80_conf-files.md`); per-platform help is an HTML file referenced by `@HELP_FILE` in each conf.
