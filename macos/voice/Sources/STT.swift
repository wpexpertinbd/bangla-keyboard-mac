// Free Bangla/English STT via Google's legacy speech-api/v2 endpoint — the same one
// the Windows build (windows/voice/stt_http.h) and the open-source recognize_google use.
// We POST 16 kHz mono LINEAR16 (audio/l16) PCM from native code (no CORS) and read back
// the transcript. The endpoint is UNOFFICIAL + rate-limited (throttles under load), so
// callers fall back (bn-BD -> bn-IN) on repeated empty results. Nothing is stored.
import Foundation

enum STT {
    // The well-known free Chromium speech key (shared, may be throttled by Google).
    static let key = "AIzaSyBOti4mM-6x9WDnZIjIeyEU21OpBXqWBgw"

    // Ephemeral session: no disk cache, no cookies, no credential storage — the audio and
    // response never touch disk, so "nothing is stored" is literally true.
    private static let session: URLSession = {
        let c = URLSessionConfiguration.ephemeral
        c.urlCache = nil
        c.httpCookieStorage = nil
        c.httpShouldSetCookies = false
        c.requestCachePolicy = .reloadIgnoringLocalCacheData
        return URLSession(configuration: c)
    }()

    /// POST raw L16 PCM (16 kHz mono) -> recognized transcript ("" on any failure/throttle).
    /// Synchronous (call off the main thread). `lang` e.g. "bn-BD", "bn-IN", "en-US".
    static func recognize(pcm: Data, rate: Int = 16000, lang: String) -> String {
        var comps = URLComponents(string: "https://www.google.com/speech-api/v2/recognize")!
        comps.queryItems = [
            URLQueryItem(name: "output", value: "json"),
            URLQueryItem(name: "lang", value: lang),
            URLQueryItem(name: "key", value: key),
        ]
        guard let url = comps.url else { return "" }
        var req = URLRequest(url: url, timeoutInterval: 15)
        req.httpMethod = "POST"
        req.httpShouldHandleCookies = false
        req.setValue("audio/l16; rate=\(rate)", forHTTPHeaderField: "Content-Type")
        req.setValue("BanglaVoice/1.1", forHTTPHeaderField: "User-Agent")
        req.httpBody = pcm

        var out = ""
        let sem = DispatchSemaphore(value: 0)
        let task = session.dataTask(with: req) { data, _, _ in
            defer { sem.signal() }
            guard let data = data, let body = String(data: data, encoding: .utf8) else { return }
            out = firstTranscript(body)
        }
        task.resume()
        sem.wait()
        return out
    }

    /// The endpoint returns line-delimited JSON; pull the first "transcript":"..." value.
    /// (Mirrors extractTranscript() in stt_http.h, but JSONDecoder handles the escapes.)
    static func firstTranscript(_ body: String) -> String {
        for line in body.split(whereSeparator: { $0 == "\n" || $0 == "\r" }) {
            guard line.contains("\"transcript\"") else { continue }
            guard let data = line.data(using: .utf8),
                  let obj = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
                  let results = obj["result"] as? [[String: Any]] else { continue }
            for r in results {
                if let alts = r["alternative"] as? [[String: Any]] {
                    for a in alts {
                        if let t = a["transcript"] as? String, !t.isEmpty { return t }
                    }
                }
            }
        }
        return ""
    }
}
