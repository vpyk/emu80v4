# Z80 CPU emulator library

Zilog Z80 CPU core by Manuel Sainz de Baranda y Goñi, used by `src/CpuZ80.cpp`.
The Z80InsnClock library is vendored here as well, see [below](#z80insnclock).

- Upstream: <https://github.com/redcode/Z80>
- Version: `v0.2-pre-2025-07-02`, commit `3c43bb0107fab5c0a4dfa720239159c5edb4ba8f`
- License: GNU LGPL v3 or later, see `COPYING.LESSER` in this directory.
  The LGPL supplements the GNU GPL v3, whose text is in `COPYING.txt` in the
  repository root.

## Files

| File | Origin |
|---|---|
| `Z80.h` | `API/Z80.h`, unmodified |
| `Z80.cpp` | `sources/Z80.c`, with local changes (see below) |
| `zdefs.h` | Emu80-specific, not part of upstream (shared with Z80InsnClock) |
| `COPYING.LESSER` | `COPYING.LESSER`, unmodified |

## Local changes

- `sources/Z80.c` is renamed to `Z80.cpp` and compiled as C++ together with the
  rest of the emulator.
- Three lines are prepended to `Z80.cpp`:
  ```c
  #define Z80_STATIC
  #define Z80_WITH_LOCAL_HEADER
  #define Z80_EXTERNAL_HEADER "zdefs.h"
  ```
  They build the library statically and make it use `zdefs.h` instead of the
  Zeta library headers (`<Z/...>`), which upstream depends on.
- Tabs in `Z80.cpp` are replaced with spaces. There are no other changes:
  `diff -w` against upstream `sources/Z80.c` shows only the three lines above.
- `zdefs.h` provides the minimal subset of Zeta types and macros that `Z80.h`
  and `Z80.cpp` need. It assumes a little-endian host.

## Updating

1. Take `API/Z80.h` and `sources/Z80.c` from the new upstream version.
2. Rename `Z80.c` to `Z80.cpp` and prepend the three `#define` lines above.
3. If the new version uses Zeta types or macros that `zdefs.h` lacks, add them there.
4. Update the version and commit in this file.

## Z80InsnClock

Instruction clock library by Sofía Ortega Sosa: determines the clock cycle,
relative to the start of the instruction, at which the ongoing M-cycle begins,
as well as the number of extra clock cycles in it. Used by `src/CpuZ80.cpp`.

- Upstream: <https://github.com/agaxia/Z80InsnClock>
- Version: `v0.1`, commit `a265388b2e72c31f60abe5ad2f65427feca873ea`
- License: 0BSD, see `LICENSE-0BSD` in this directory.

| File | Origin |
|---|---|
| `Z80InsnClock.h` | `API/Z80InsnClock.h`, unmodified |
| `Z80InsnClock.cpp` | `sources/Z80InsnClock.c`, with local changes (see below) |
| `LICENSE-0BSD` | `LICENSE-0BSD`, unmodified |

Local changes:

- `sources/Z80InsnClock.c` is renamed to `Z80InsnClock.cpp` and compiled as C++.
- Five lines are prepended to `Z80InsnClock.cpp`:
  ```c
  #define Z80_INSN_CLOCK_STATIC
  #define Z80_INSN_CLOCK_WITH_LOCAL_HEADER
  #define Z80_INSN_CLOCK_EXTERNAL_HEADER "zdefs.h"
  #define Z80_EXTERNAL_HEADER "zdefs.h"
  #define Z80_INSN_CLOCK 2
  ```
  `Z80_INSN_CLOCK 2` builds only the Extra mode (the Basic mode table is not
  compiled). Upstream `Z80InsnClock.h` tests `Z80_INSN_CLOCK_EXTERNAL_HEADER`
  but includes `Z80_EXTERNAL_HEADER`, so both are defined.
- Tabs in `Z80InsnClock.cpp` are expanded to spaces (tab width 8, keeping the
  tables aligned) and trailing whitespace is removed; `diff -w` against upstream
  shows only the lines above.

Updating: take `API/Z80InsnClock.h` and `sources/Z80InsnClock.c`, repeat the
changes above, add any missing Zeta macros to `zdefs.h`, update the commit here.

Usage notes (Extra mode, as used by `CpuZ80`):

- The tables assume that `z80_run()` is called for one instruction at a time:
  a DDh/FDh prefix always ends the step (`Z80_RESUME_XY`), and the opcode that
  follows it starts the next step at cycle 0.
- `z80_insn_clock_start()` before each `z80_run()` with `Z80::resume`; in the
  opcode fetch callback `z80_insn_clock_extra_m1()` → access →
  `z80_insn_clock_extra_add_m1()`; in memory read/write callbacks
  `z80_insn_clock_extra_m()` → access → `z80_insn_clock_extra_add_m()`; in I/O
  callbacks only `z80_insn_clock_extra_add_in()` (the I/O M-cycle always takes
  4 cycles; `z80_insn_clock_extra_out()` gives the extra cycles after an OUT);
  in INTA `z80_insn_clock_extra_inta()` → `z80_insn_clock_extra_add_inta()`.
  `Z80InsnClock::cycles` between the first and second call is the cycle at
  which the current M-cycle begins.
