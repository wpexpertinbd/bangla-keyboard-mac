# Bangla Keyboard — Windows port

> **Status: shipping — the tray app is released (`win-v1.1.0`); the TSF IME is experimental.**
> - ✅ **Engine ported to C++** ([`engine/`](engine/)) — **13/13** SPEC §7 corpus
>   (compiled `enginetest.exe`; `engine/verify.py` is a Python mirror).
> - ✅ **TSF text service** ([`tsf/`](tsf/)) — builds to `BanglaKeyboard.dll` (x64)
>   **and `BanglaKeyboard32.dll` (x86)**; both load + COM/TSF vtables verified via
>   `loadtest.exe` / `loadtest32.exe` (no registry changes).
>   ⬜ Live in-app typing, composing underline, and signing still to do.
> - ✅ **Tray switcher** ([`tray/`](tray/)) — `bangla-tray.exe`: a system-tray
>   icon + menu (**Bangla Unicode / Bangla Classic / English**) + Ctrl+Alt+B,
>   types Bangla in any app via a global hook. No install/admin. Builds & runs.
> - ✅ **Exact macOS parity** ([`engine/klengine.*`](engine/)) — both Bangla Unicode
>   and Bangla Classic are driven by FSM tables generated from the Mac `.keylayout`
>   files, so output is byte-identical to the Mac (20/20 + 12/12 vs the validated
>   interpreter). Independent vowels match too: `f`→া, `Shift+f`→অ, `Shift+f f`→আ.
> - ✅ **Voice typing** ([`voice/`](voice/)) — `bangla-voice.exe`: speak **Bangla
>   (Ctrl+Alt+S)** or **English (Ctrl+Alt+D)** and it types at the cursor. Free online
>   STT — correct Bangladeshi **bn-BD** via Google's legacy endpoint (POSTed from
>   native code), English via the browser **Web Speech** API; an off-screen WebView2
>   captures the mic, a tray icon shows state, "।"/"." is auto-appended. Opt-in at
>   install. See [`SECURITY.md`](../SECURITY.md) for the privacy/network audit.
> - ✅ **Runnable demo** — `bangla-demo.exe`: type on your keyboard, see live Bangla.
> - 🟡 **Installer** ([`installer/`](installer/)) — `register.bat`/`unregister.bat`
>   work; a packaged signed installer is still TODO.
>
> Build everything: [`build-all.bat`](build-all.bat) (needs MinGW g++ — w64devkit —
> or MSVC). Artifacts + usage: [`dist/README.md`](dist/README.md).
>
> ⚠️ Found a reph-reordering bug in the reference `engine/Engine.swift` while porting
> (`ভার্সন`/`কর্ম` fail a faithful port); fixed in both the C++ engine and
> `engine/Engine.swift` per the macOS `.keylayout` ground truth. See
> [`engine/README.md`](engine/README.md).

## Read first
1. [`../SPEC.md`](../SPEC.md) — the complete, OS-neutral engine spec (keymap + algorithm +
   test corpus). **This is the contract.** Your build must reproduce it exactly.
2. [`../engine/Engine.swift`](../engine/Engine.swift) — the 172-line reference engine. Port
   this logic to C++ (or C#). It is the source of truth; if SPEC.md and Engine.swift ever
   disagree, Engine.swift wins.
3. [`../macos/`](../macos/) — the shipping macOS version, for behaviour comparison.

## Why a TSF IME (and not an MSKLC `.klc` layout)
The keyboard's whole point is **reordering one syllable** so a prebase vowel typed *before*
its consonant lands in correct Unicode order (`ে`+`ক`→`কে`), plus reph-after-consonant
(`ভার্সন`) and prebase-through-conjunct (`ক্ষি`). A static Windows layout can do single
deadkeys only — it **cannot** do this. So the real port is a **TSF text service** that runs
the engine. (A `.klc` may ship as a clearly-labelled "basic, no reordering" fallback, but it
is not the deliverable.)

## Build plan (suggested)
1. **Port the engine headless first.** Create `windows/engine/engine.cpp` + `engine.h` as a
   pure module (no Windows headers), then `windows/engine/test.cpp` that feeds the §7 corpus
   and asserts NFC output. Get all tests green before any COM/TSF code.
   - Watch the **hasanta grapheme trap** (test last code unit == U+09CD, not string suffix).
2. **TSF shell.** A 64-bit (and 32-bit) in-proc COM DLL implementing
   `ITfTextInputProcessorEx`, `ITfThreadMgrEventSink`, `ITfKeyEventSink`,
   `ITfCompositionSink`. Map engine results → composition string / commit
   (see SPEC §2 and §8). Bind keys by **scan code** for the US-QWERTY positions in SPEC §6
   so it works under any base layout; flush + pass through on Ctrl/Alt/Win chords.
3. **Register + install.** Register the TIP (CLSID + `bn-BD` language profile) under
   `HKLM\SOFTWARE\Microsoft\CTF\TIP`. Ship an installer (MSIX or a small EXE doing
   `ITfInputProcessorProfiles::Register` + DLL registration). Code-sign to avoid SmartScreen.
4. **Verify** against SPEC §7 on a real Windows box (Notepad + a Unicode app), then compare
   side-by-side with the macOS build.

## Layout this folder like
```
windows/
├── engine/        # C++ engine + FSM tables (KLEngine) + headless tests
├── tray/          # bangla-tray.exe — the shipped tray switcher
├── voice/         # bangla-voice.exe — online voice typing (WebView2 + Google STT)
├── tsf/           # experimental TSF TIP COM DLL (not shipped)
├── installer/     # Inno Setup installer (.iss)
└── README.md
```

## Don'ts
- No trademarked brand names anywhere; no bundled commercial fonts (engine emits standard Unicode, so
  no fonts are needed — any Bangla Unicode font renders it).
- Don't hand-port the macOS `.keylayout` deadkey machine — port the **engine**, it's cleaner.
