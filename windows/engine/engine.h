// Bangla Keyboard — reordering engine (Windows port).
//
// Pure C++ module, no Windows headers, so it can be unit-tested headless and
// reused unchanged inside the TSF text service. Port of ../../engine/Engine.swift
// (the cross-platform reference) keyed by Windows Set-1 scan codes instead of
// macOS virtual key codes. See ../../SPEC.md for the full contract.
//
// Strings are UTF-16 (std::u16string) — the same encoding TSF uses (WCHAR), and
// because every Bengali character this engine emits lives in the BMP, one UTF-16
// code unit == one Unicode scalar. That makes the "last scalar == U+09CD"
// hasanta test and the "single consonant code point" test exact, avoiding the
// grapheme-cluster trap called out in SPEC §3.
//
// NOTE (reph reordering): Engine.swift appends reph (cons += "র্"), which does
// NOT reorder and fails SPEC §7's ভার্সন / কর্ম. The shipping macOS .keylayout
// (the verified ground truth) DOES reorder — state "ka" + reph -> "র্ক". This
// port follows the .keylayout / SPEC §7 and reorders reph to the front of a
// closed consonant. This is a deliberate one-line divergence from Engine.swift's
// bug; see processClusterMod().

#ifndef BANGLA_ENGINE_H
#define BANGLA_ENGINE_H

#include <string>

namespace bangla {

using Str = std::u16string;

enum class ResultKind {
    Ignore,            // not our key — host should handle the event
    Compose,           // show `text` as marked (composing) text
    Commit,            // insert `text` as final; buffer cleared
    CommitThenCompose, // insert `text` as final, then begin composing `compose`
};

struct Result {
    ResultKind kind = ResultKind::Ignore;
    Str text;  // Compose/Commit payload, or the "final" part of CommitThenCompose
    Str comp;  // the new composing run (CommitThenCompose only)

    static Result ignore()                       { return {ResultKind::Ignore, {}, {}}; }
    static Result compose(Str t)                 { return {ResultKind::Compose, std::move(t), {}}; }
    static Result commit(Str t)                  { return {ResultKind::Commit, std::move(t), {}}; }
    static Result commitThenCompose(Str f, Str c){ return {ResultKind::CommitThenCompose, std::move(f), std::move(c)}; }
};

class Engine {
public:
    // Process one key. `scanCode` is the Windows Set-1 make code for the physical
    // key (lParam bits 16-23 of WM_KEYDOWN); `shift` is whether Shift is held.
    // Keys are bound by physical position so the layout works under any base
    // keyboard. VK_BACK (scan 0x0E) edits the composing buffer.
    Result process(unsigned scanCode, bool shift);

    // True if process() would consume this key right now — a mapped layout key,
    // or Backspace (0x0E) while composing. Use from TSF OnTestKeyDown to claim
    // the key without committing to handling it. Does not mutate state.
    bool wouldHandle(unsigned scanCode) const;

    // Emit renderFinal() and reset. Call on focus loss, Ctrl/Alt/Win chords,
    // app switch, etc. — the host should flush before handling a non-text key.
    Str flush();

    bool empty() const { return cons_.empty() && vowel_.empty() && trail_.empty(); }
    void reset() { cons_.clear(); vowel_.clear(); trail_.clear(); }

private:
    Str cons_;   // consonant cluster, may end in hasanta ্ mid-cluster
    Str vowel_;  // the (combined) vowel sign
    Str trail_;  // trailing signs ং ঃ ঁ

    Str render() const { return cons_ + vowel_ + trail_; }
    Str renderFinal() const;             // lone vowel sign -> independent vowel
    static bool endsWithHasanta(const Str& s) { return !s.empty() && s.back() == 0x09CD; }
    static bool isConsonant(const Str& u);
    static Str combineVowel(const Str& existing, const Str& add);
    void popLast();
};

} // namespace bangla

#endif // BANGLA_ENGINE_H
