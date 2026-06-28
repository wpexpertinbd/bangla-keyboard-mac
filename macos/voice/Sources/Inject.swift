// Insert recognized text at the cursor in whatever app is focused, by posting Unicode
// key events (port of injectText() in windows/voice/voicehost.cpp — SendInput KEYEVENTF_UNICODE).
// Requires Accessibility permission (posting events to other apps). Control chars are
// never injected, so a bad STT response can't fire Return/Tab/etc.
import CoreGraphics
import Foundation

enum Inject {
    static func text(_ s: String) {
        let src = CGEventSource(stateID: .hidSystemState)
        for scalar in s.unicodeScalars {
            if scalar.value < 0x20 { continue }               // strip control chars
            // UTF-16 units for this scalar (handles astral chars too).
            let units = Array(String(scalar).utf16)
            post(units, keyDown: true, src: src)
            post(units, keyDown: false, src: src)
        }
    }

    private static func post(_ units: [UniChar], keyDown: Bool, src: CGEventSource?) {
        guard let ev = CGEvent(keyboardEventSource: src, virtualKey: 0, keyDown: keyDown) else { return }
        var u = units
        ev.keyboardSetUnicodeString(stringLength: u.count, unicodeString: &u)
        ev.post(tap: .cgSessionEventTap)
    }
}
