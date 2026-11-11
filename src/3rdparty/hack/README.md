# Hack font

Monospaced font by Source Foundry, used by the Qt debugger window
(`src/qt/qtDbgStyle.cpp`). It is embedded into the executable as a Qt resource
(`src/qt/dbgfont.qrc`) and is not installed in the system.

- Upstream: <https://github.com/source-foundry/Hack>
- Version: `3.003`
- License: MIT License for the Hack work, Bitstream Vera License for the
  Bitstream Vera Sans Mono glyphs it is based on, see `LICENSE.md` in this directory.
  The license text is also included in `THIRD-PARTY-LICENSES.txt` in the
  repository root, which is installed with the emulator.

## Files

| File | Origin |
|---|---|
| `Hack-Regular.ttf` | `build/ttf/Hack-Regular.ttf` of the v3.003 release, unmodified |
| `LICENSE.md` | `LICENSE.md` of the v3.003 release, unmodified |
