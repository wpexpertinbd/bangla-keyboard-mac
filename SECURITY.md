# Security

## Reporting

Found a security issue? Please open a GitHub issue (or contact BiswasHost at
<https://www.biswashost.com>). We'll respond promptly.

## Security posture

This project is a macOS keyboard‑layout installer. It has a deliberately small
attack surface:

- **No network access, no telemetry, no analytics, no auto‑update.**
- **No credentials handled or stored.**
- The only privileged actions are: installing two `.keylayout` files + free
  fonts into `/Library`, and (optionally) removing them again.

### Code that runs with elevated privileges

| Component | Runs as | Notes |
|---|---|---|
| `src/build/scripts/preinstall` | root (during `.pkg` install) | Only `rm -f` of **this project's own** hard‑coded layout paths. No dynamic input. |
| `src/build/scripts/postinstall` | root (during `.pkg` install) | Only absolute‑path `atsutil` calls to refresh the font cache. No dynamic input. |
| `Bangla Keyboard Installer.app` (`src/build/installer.applescript`) | admin (via `osascript … with administrator privileges`) | Install/Reinstall/Uninstall. All filesystem paths are passed to AppleScript as **arguments** and shell‑escaped with `quoted form of` — see below. |

### Injection‑safe privileged execution

The installer app never interpolates a path directly into a privileged shell
string. Paths are passed as `osascript` arguments and quoted by AppleScript
itself:

```applescript
on run argv
    do shell script "/usr/sbin/installer -pkg " & quoted form of (item 1 of argv) & " -target /" with administrator privileges
end run
```

This means a path containing quotes, spaces, or shell metacharacters cannot
break out of the command (verified with a `'; touch …; '` payload during
development). The uninstall path uses the same pattern for every file it removes.

### Data files

- The `.keylayout` files are XML consumed by macOS's text‑input engine. They
  contain only the standard system DTD reference (no external entities / XXE)
  and key→character mappings. The low‑byte `&#x00..;` outputs in **Bangla
  Classic** are legacy font glyph codes — data, never executed.
- Fonts are free/libre TrueType files from the projects listed in
  `fonts-licenses/FONTS.md`.

## Windows (tray app)

The Windows build (`windows/`, shipped as `bangla-tray.exe`) is a tray keyboard
that uses a global low-level keyboard hook (`WH_KEYBOARD_LL`) — the standard way an
input method sees keystrokes. Audit findings:

- **No keystroke logging, storage, or transmission.** The app holds only the
  *current syllable* in memory and clears it on every word boundary. Source review:
  **no file, network, registry, or process-exec calls** anywhere in the app or
  engine (`grep` for `CreateFile`/`WriteFile`/`socket`/`WinHttp`/`RegSetValue`/
  `ShellExecute`/`system` → none). No telemetry, no auto-update, no analytics.
- **No DLL-hijacking surface.** The binary is statically linked and imports **only
  system DLLs** (`kernel32`, `user32`, `gdi32`, `shell32`, `msvcrt`), which load
  from `System32` / KnownDLLs — no third-party or app-directory DLL is loaded.
- **Least privilege.** Runs as the normal user (no elevation); the installer is
  **per-user** (`%LocalAppData%`), so installing/uninstalling needs no admin.
- **Exception-safe hook.** The hook callback is wrapped so no C++ exception can
  escape into the OS input dispatch (a crash there would affect every app); on any
  error it drops its pending state and stays alive.
- **Bounded buffers.** The in-progress run is capped (1024 chars) so a long burst or
  stuck auto-repeat can't grow memory/CPU unbounded.
- **Engine input safety.** The reordering engine is driven by **compile-time-constant
  tables** generated from the macOS `.keylayout` files; the only runtime input is the
  user's own scan codes (bounded to one byte). No untrusted data crosses into it.
- **Installer.** No remote code/downloads; the only external call is a fixed
  `taskkill /im bangla-tray.exe` (constant image name — no injection) to close the
  running tray before install/uninstall.

The optional **TSF IME** DLL (`windows/tsf/`, not shipped in the release) is an
in-proc COM keyboard service; it is experimental and excluded from binary releases
until reviewed and code-signed.

## Known / accepted items

- **Windows: unsigned + keylogger-shaped tech.** The tray app is not code-signed
  yet (SmartScreen warns; *More info → Run anyway*), and a global keyboard hook is
  the same Windows API a keylogger uses — so some antivirus may heuristically flag
  it. It does **not** exfiltrate anything (see the audit above); a code-signing
  certificate + reputation removes both the SmartScreen prompt and most AV noise.

- **Unsigned distribution.** The `.pkg` and `.command` are not code‑signed or
  notarized, so Gatekeeper shows an "unidentified developer" warning and users
  must right‑click → **Open** the first time. If you build a signed/notarized
  copy with an Apple Developer ID, that warning goes away. This is a trust/UX
  item, not a code vulnerability.
- Because installation writes to `/Library`, an admin password is required —
  this is expected for system‑wide keyboard layouts and fonts.
