# Bangla Keyboard — Windows build artifacts

Built by [`../build-all.bat`](../build-all.bat) / [`../build-all.sh`](../build-all.sh).
The binaries themselves are git-ignored (rebuild them locally); this folder holds:

| artifact | what it is | how to use |
|---|---|---|
| `enginetest.exe` | the SPEC §7 corpus test | double-click / run → expect **13/13 passed** |
| `bangla-demo.exe` | standalone typing demo (not the IME) | run it, type on your US-QWERTY keyboard, watch live Bangla. `[ ]` marks the composing syllable. **Enter** = new line, **Esc** = quit. Batch mode: `bangla-demo.exe --keys "c j f"` → `কো` |
| `BanglaKeyboard.dll` | **the IME** — a TSF text service (x64) | register it (below), then it's a system keyboard you can switch to with **Win+Space** in any app |
| `loadtest.exe` | non-destructive DLL check | run from this folder → loads the DLL, creates the COM object, QIs the TSF sinks. **No registry changes.** Expect **LOADTEST PASS** |

## Install the IME (needs Administrator)
```
..\installer\register.bat       (right-click → Run as administrator)
```
Then add it: **Settings → Time & language → Language & region → Bangla →
Language options → add a keyboard → "Bangla Keyboard (Unicode)"**, or switch with
**Win+Space**. Remove with `..\installer\unregister.bat` (as admin).

## Status / caveats
- ✅ Engine: **13/13** SPEC corpus (compiled `enginetest.exe`).
- ✅ DLL builds, loads, and its COM/TSF vtables verify (`loadtest.exe`).
- ⬜ **Live in-app typing not yet verified** — that needs registering the IME on a
  real Windows session and typing in Notepad/Word. Do that to fully sign it off.
- ⬜ **x64 only.** 32-bit apps need a Win32 build of the DLL too (build with a
  32-bit toolchain or MSVC `-A Win32`); see `../tsf/README.md`.
- ⬜ Composing-text **underline** (display attribute) is still a TODO — composition
  works without it; it's cosmetic. See `../tsf/README.md`.
- ⬜ **Unsigned.** Code-sign before distributing (SmartScreen). See `../installer/`.
