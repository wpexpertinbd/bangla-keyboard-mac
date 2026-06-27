// Bangla Keyboard — standalone demo (NOT the IME; that's the TSF DLL in ../tsf/).
//
// Proves the engine in a runnable .exe. Two modes:
//   bangla-demo.exe                 interactive: type on your US-QWERTY keyboard
//                                   and watch the engine produce live Bangla.
//                                   Enter = new line, Esc = quit.
//   bangla-demo.exe --keys "c j f"  batch: feed a key spec (^ = Shift,
//                                   space-separated) and print the result.
//
// The engine is shared verbatim with the TSF text service.
#include "engine.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <cstring>

using bangla::Engine;
using bangla::Result;
using bangla::ResultKind;
using bangla::Str;

static HANDLE hOut;

static std::string toUtf8(const Str& s) { // BMP-only is enough here
    std::string out;
    for (char16_t c : s) {
        unsigned v = c;
        if (v < 0x80) out += char(v);
        else if (v < 0x800) { out += char(0xC0 | (v >> 6)); out += char(0x80 | (v & 0x3F)); }
        else { out += char(0xE0 | (v >> 12)); out += char(0x80 | ((v >> 6) & 0x3F)); out += char(0x80 | (v & 0x3F)); }
    }
    return out;
}

// WriteConsoleW only works on a real console; when stdout is redirected/piped it
// fails, so fall back to writing UTF-8 bytes via WriteFile.
static void writeW(const Str& s) {
    DWORD n = 0;
    if (GetFileType(hOut) == FILE_TYPE_CHAR && WriteConsoleW(hOut, s.c_str(), (DWORD)s.size(), &n, nullptr)) return;
    std::string u8 = toUtf8(s);
    WriteFile(hOut, u8.data(), (DWORD)u8.size(), &n, nullptr);
}
static void writeA(const char* s) {
    DWORD n = 0, len = (DWORD)std::strlen(s);
    if (GetFileType(hOut) == FILE_TYPE_CHAR && WriteConsoleA(hOut, s, len, &n, nullptr)) return;
    WriteFile(hOut, s, len, &n, nullptr);
}

// US-QWERTY char -> Windows scan code (mirrors test.cpp / SPEC §6).
static unsigned scanOf(char c) {
    switch (c) {
        case '`': return 0x29; case '1': return 0x02; case '2': return 0x03;
        case '3': return 0x04; case '4': return 0x05; case '5': return 0x06;
        case '6': return 0x07; case '7': return 0x08; case '8': return 0x09;
        case '9': return 0x0A; case '0': return 0x0B; case '-': return 0x0C; case '=': return 0x0D;
        case 'q': return 0x10; case 'w': return 0x11; case 'e': return 0x12; case 'r': return 0x13;
        case 't': return 0x14; case 'y': return 0x15; case 'u': return 0x16; case 'i': return 0x17;
        case 'o': return 0x18; case 'p': return 0x19; case '[': return 0x1A; case ']': return 0x1B;
        case '\\': return 0x2B;
        case 'a': return 0x1E; case 's': return 0x1F; case 'd': return 0x20; case 'f': return 0x21;
        case 'g': return 0x22; case 'h': return 0x23; case 'j': return 0x24; case 'k': return 0x25;
        case 'l': return 0x26; case ';': return 0x27; case '\'': return 0x28;
        case 'z': return 0x2C; case 'x': return 0x2D; case 'c': return 0x2E; case 'v': return 0x2F;
        case 'b': return 0x30; case 'n': return 0x31; case 'm': return 0x32; case ',': return 0x33;
        case '.': return 0x34; case '/': return 0x35;
        default:  return 0;
    }
}

// Apply a Result to a (committed, composing) pair the way the TSF shell would.
static void apply(const Result& r, Str& committed, Str& composing) {
    switch (r.kind) {
        case ResultKind::Ignore: break;
        case ResultKind::Compose: composing = r.text; break;
        case ResultKind::Commit: committed += r.text; composing.clear(); break;
        case ResultKind::CommitThenCompose: committed += r.text; composing = r.comp; break;
    }
}

static int batch(const char* spec) {
    Engine e;
    Str committed, composing;
    std::string tok;
    auto play = [&](const std::string& t) {
        if (t.empty()) return;
        bool shift = t[0] == '^';
        char key = shift ? t[1] : t[0];
        apply(e.process(scanOf(key), shift), committed, composing);
    };
    for (const char* p = spec; *p; ++p) { if (*p == ' ') { play(tok); tok.clear(); } else tok += *p; }
    play(tok);
    committed += e.flush();
    writeW(committed);
    writeA("\n");
    return 0;
}

static void redraw(const Str& committed, const Str& composing) {
    // \r + clear the line, then "committed[composing]" so the composing syllable
    // is visible (the IME shows it underlined instead of in brackets).
    writeA("\r");
    writeA("\x1b[2K"); // clear line (VT)
    writeA("  ");
    writeW(committed);
    if (!composing.empty()) { writeA("["); writeW(composing); writeA("]"); }
}

static int interactive() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    DWORD oldMode = 0;
    GetConsoleMode(hIn, &oldMode);
    SetConsoleMode(hIn, ENABLE_WINDOW_INPUT); // raw: no line buffering / echo
    DWORD outMode = 0;
    GetConsoleMode(hOut, &outMode);
    SetConsoleMode(hOut, outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    writeA("Bangla Keyboard - engine demo\n");
    writeA("Type on your US-QWERTY keyboard (Shift works). Enter = new line, Esc = quit.\n");
    writeA("[ ] = the composing syllable (the IME underlines it instead).\n\n");

    Engine e;
    Str committed, composing;
    redraw(committed, composing);

    INPUT_RECORD ir;
    DWORD nread = 0;
    bool running = true;
    while (running && ReadConsoleInputW(hIn, &ir, 1, &nread) && nread) {
        if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown) continue;
        const KEY_EVENT_RECORD& k = ir.Event.KeyEvent;
        WORD vk = k.wVirtualKeyCode;
        unsigned scan = k.wVirtualScanCode;
        DWORD ctl = k.dwControlKeyState;
        bool shift = (ctl & SHIFT_PRESSED) != 0;
        bool ctrlAlt = (ctl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED | LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)) != 0;

        if (vk == VK_ESCAPE) { committed += e.flush(); running = false; break; }
        if (vk == VK_RETURN) { committed += e.flush(); writeW(composing = Str()); writeA("\r\n"); committed.clear(); redraw(committed, composing); continue; }
        if (ctrlAlt) { committed += e.flush(); composing.clear(); redraw(committed, composing); continue; }

        if (e.wouldHandle(scan)) {
            apply(e.process(scan, shift), committed, composing);
            redraw(committed, composing);
        } else if (vk == VK_BACK && composing.empty() && !committed.empty()) {
            committed.pop_back(); // edit already-committed text
            redraw(committed, composing);
        }
    }

    SetConsoleMode(hIn, oldMode);
    writeA("\n");
    return 0;
}

int main(int argc, char** argv) {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);
    if (argc >= 3 && std::strcmp(argv[1], "--keys") == 0) return batch(argv[2]);
    if (argc >= 2 && std::strcmp(argv[1], "--keys") == 0) { writeA("usage: bangla-demo --keys \"c j f\"\n"); return 2; }
    return interactive();
}
