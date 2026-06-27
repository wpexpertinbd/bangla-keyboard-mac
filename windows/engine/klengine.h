// Bangla Keyboard — table-driven keylayout engine.
//
// Runs an Apple .keylayout deadkey FSM (the same algorithm as
// keylayout_interp.py), driven by a generated table. Both layouts use it:
//   unicode_table.h  — standard Unicode Bangla   (bangla::unicode_table::TABLE)
//   classic_table.h  — legacy ANSI Bangla        (bangla::classic_table::TABLE)
//
// Reproduces the macOS .keylayout EXACTLY, so f→া, Shift+f→অ, Shift+f then f→আ,
// reph reorders, etc. — no hand-written rules to diverge.
//
// APPEND-ONLY: process() returns the text to APPEND for this key (often empty
// while a deadkey is pending); flush() emits the dangling deadkey on a boundary.
// There is no back-spacing, so injection is robust in every app. Strings are
// std::u16string (every code point is BMP).
#ifndef BANGLA_KLENGINE_H
#define BANGLA_KLENGINE_H

#include <string>

namespace bangla {

struct Rule { short state; short out; short next; };
struct Span { short off;  short cnt; };

struct Table {
    const char16_t* const* strings;   // interned output strings ("" = id 0)
    const Rule*            rules;
    const Span*            actions;    // per-action span into rules
    const short*           term;       // terminator output id per state
    const short          (*keymap)[256]; // [shift 0..1][scancode] -> action id, or -1
};

class KLEngine {
public:
    explicit KLEngine(const Table* t) : t_(t) {}
    // Windows Set-1 scan code + Shift -> text to append (may be empty).
    std::u16string process(unsigned scanCode, bool shift);
    // Emit any dangling deadkey and reset (space / Enter / focus loss / chord).
    std::u16string flush();
    // What the current pending deadkey would emit if flushed NOW — used to show a
    // live preview of the in-progress syllable (so characters appear immediately,
    // not one key late). Does not change state.
    std::u16string peek() const { return t_->strings[t_->term[state_]]; }
    void reset() { state_ = 0; }
    // True if this scan code is part of the layout (else the host handles it).
    bool wouldHandle(unsigned scanCode) const;

private:
    const Table* t_;
    int state_ = 0;   // 0 = "none"
};

} // namespace bangla

#endif
