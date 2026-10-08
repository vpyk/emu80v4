# Z80 CPU emulator library

Zilog Z80 CPU core by Manuel Sainz de Baranda y Goñi, used by `src/CpuZ80.cpp`.

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
| `zdefs.h` | Emu80-specific, not part of upstream |
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
