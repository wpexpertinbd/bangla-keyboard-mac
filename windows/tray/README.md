# Bangla Keyboard — tray switcher

`bangla-tray.exe` is a standalone background app with a **system-tray icon + popup
menu** (Bangla Unicode / Bangla Classic / English) — the same two layouts the
macOS build ships, switchable from the tray. It does **not** need the TSF IME,
registration, or admin: just run the .exe.

## The three modes
- **Bangla Unicode** (অ on a red circle) — standard Unicode Bangla, works with any
  Bangla font. Driven by the Mac Unicode `.keylayout` FSM (`../engine/unicode_table.h`).
- **Bangla Classic** (ক on a brick-red circle) — the **legacy ANSI Bangla**
  encoding, byte-identical to the macOS Classic layout. Run through the Classic FSM
  (`../engine/classic_table.h`, generated from the Mac `.keylayout`).
  ⚠️ Renders as Bangla **only in a legacy ANSI Bangla font** (the kind used for old
  documents) — set your document/app font to one. We don't bundle one (commercial).
- **English** (E on a grey circle) — passthrough.

The tray icons match the macOS build: a green square (Bangladesh-flag green) + a
coloured circle + a white Bangla letter.

## How it works
- Switch with: **left-click the tray icon** (toggles English ⇄ last Bangla mode),
  the **right-click menu**, or **Ctrl+Alt+B**.
- A global low-level keyboard hook (`WH_KEYBOARD_LL`) runs each keystroke through
  the keylayout-driven engine ([`../engine/klengine.*`](../engine/)) and injects the
  result with `SendInput`, so it works in **any** app (Notepad, Word, browsers, chat).
- **Live preview:** the deadkey FSM defers, so the tray shows the pending character
  immediately (`committed + peek()`) and back-spaces only the part that actually
  changes on a reorder (e.g. `ে`→`কে`, reph). So every letter appears as you type —
  no one-key lag — while prebase-vowel/reph reordering still works.
- Output is **byte-identical to the macOS build** (same `.keylayout` FSM), incl.
  independent vowels: `f`→া, `Shift+f`→অ, `Shift+f` then `f`→আ.

## Run it
1. Build (see [`../build-all.bat`](../build-all.bat)) → `../dist/bangla-tray.exe`.
2. Double-click it. A tray icon appears (starts in **English**).
3. Press **Ctrl+Alt+B** (or click the icon) to switch to **Bangla**, then type.
4. Right-click the icon → **Close** to quit.

To start it automatically at login, drop a shortcut to `bangla-tray.exe` in
`shell:startup` (Win+R → `shell:startup`).

## Tray vs IME — which one?
| | tray app (`bangla-tray.exe`) | TSF IME (`../tsf/`) |
|---|---|---|
| Install | none — just run | `regsvr32` (admin), add keyboard |
| Switch | tray icon / Ctrl+Alt+B | Win+Space (Windows switcher) |
| Works in | every app (global hook + inject) | every TSF-aware app (composition) |
| Robustness | edits text by injecting backspaces — can be off in apps with grapheme-cluster backspace, password boxes, or some games | proper IME composition; the "correct" Windows way |
| Modes | Unicode + Classic + English | Unicode (Classic could be added) |

Both share the **same engine**, so typing behaviour matches. Use the tray app for
the switch-from-the-tray experience; use the IME for the most robust integration.

## Limitations / TODO
- **Classic needs a legacy ANSI Bangla font** to render (we can't bundle one).
  Without it the text shows as Latin/symbol characters, same as on Mac.
- Live preview back-spaces only on reorders (usually 0–1 per key), so it's robust —
  but a global hook + `SendInput` can still be blocked in password fields or some
  full-screen games, and apps that delete a whole grapheme cluster per backspace may
  over-delete on a reorder.
- x64 only — a global hook works across all apps regardless of their bitness, so no
  separate 32-bit build is needed.
- Unsigned — code-sign before distributing (a keyboard hook + unsigned exe will
  draw SmartScreen / AV attention).
