#include "TextService.h"
#include <new>

using bangla::Engine;
using bangla::Result;
using bangla::ResultKind;
using bangla::Str;

// ---------------------------------------------------------------------------
// Small helpers (run only inside an edit session, i.e. with a valid TfEditCookie)
// ---------------------------------------------------------------------------
namespace {

HRESULT StartComposition(TextService* ts, TfEditCookie ec, ITfContext* pic) {
    if (ts->composition()) return S_OK; // already composing
    HRESULT hr = E_FAIL;
    ITfInsertAtSelection* pias = nullptr;
    if (FAILED(pic->QueryInterface(IID_ITfInsertAtSelection, (void**)&pias))) return E_FAIL;
    ITfRange* pRange = nullptr;
    // empty range at the caret
    if (SUCCEEDED(pias->InsertTextAtSelection(ec, TF_IAS_QUERYONLY, nullptr, 0, &pRange)) && pRange) {
        ITfContextComposition* pcc = nullptr;
        if (SUCCEEDED(pic->QueryInterface(IID_ITfContextComposition, (void**)&pcc))) {
            ITfComposition* pComp = nullptr;
            hr = pcc->StartComposition(ec, pRange, ts /*ITfCompositionSink*/, &pComp);
            if (SUCCEEDED(hr) && pComp) ts->setComposition(pComp); // keep the ref
            pcc->Release();
        }
        pRange->Release();
    }
    pias->Release();
    return hr;
}

// Replace the composition's text and move the caret to the end of it.
HRESULT SetText(TextService* ts, TfEditCookie ec, ITfContext* pic, const Str& text) {
    ITfComposition* pComp = ts->composition();
    if (!pComp) return E_FAIL;
    ITfRange* pRange = nullptr;
    if (FAILED(pComp->GetRange(&pRange)) || !pRange) return E_FAIL;

    HRESULT hr = pRange->SetText(ec, 0, reinterpret_cast<const WCHAR*>(text.c_str()),
                                 static_cast<LONG>(text.size()));
    // VERIFY: set GUID_PROP_ATTRIBUTE on pRange to our display-attribute atom so
    // the composing run is underlined. Requires an ITfDisplayAttributeProvider
    // registered for c_guidDisplayAttribute (see DisplayAttribute TODO in README).

    // collapse the selection to the end of the composing range
    ITfRange* pEnd = nullptr;
    if (SUCCEEDED(pRange->Clone(&pEnd)) && pEnd) {
        pEnd->Collapse(ec, TF_ANCHOR_END);
        TF_SELECTION sel; sel.range = pEnd; sel.style.ase = TF_AE_NONE; sel.style.fInterimChar = FALSE;
        pic->SetSelection(ec, 1, &sel);
        pEnd->Release();
    }
    pRange->Release();
    return hr;
}

HRESULT EndComposition(TextService* ts, TfEditCookie ec) {
    ITfComposition* pComp = ts->composition();
    if (!pComp) return S_OK;
    pComp->EndComposition(ec);
    pComp->Release();
    ts->setComposition(nullptr);
    return S_OK;
}

HRESULT ApplyInSession(TextService* ts, TfEditCookie ec, ITfContext* pic, const Result& r) {
    switch (r.kind) {
        case ResultKind::Ignore:
            return S_OK;
        case ResultKind::Compose:
            if (FAILED(StartComposition(ts, ec, pic))) return E_FAIL;
            return SetText(ts, ec, pic, r.text);
        case ResultKind::Commit:
            if (FAILED(StartComposition(ts, ec, pic))) return E_FAIL;
            SetText(ts, ec, pic, r.text);
            return EndComposition(ts, ec);
        case ResultKind::CommitThenCompose:
            if (FAILED(StartComposition(ts, ec, pic))) return E_FAIL;
            SetText(ts, ec, pic, r.text);
            EndComposition(ts, ec);
            if (FAILED(StartComposition(ts, ec, pic))) return E_FAIL;
            return SetText(ts, ec, pic, r.comp);
    }
    return S_OK;
}

// Edit session that carries either an engine Result to apply, or a flush request.
class CEditSession final : public ITfEditSession {
public:
    CEditSession(TextService* ts, ITfContext* pic, const Result& r)
        : m_cRef(1), m_ts(ts), m_pic(pic), m_result(r), m_flush(false) { m_pic->AddRef(); }
    CEditSession(TextService* ts, ITfContext* pic, bool /*flushTag*/)
        : m_cRef(1), m_ts(ts), m_pic(pic), m_flush(true) { m_pic->AddRef(); }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfEditSession)) {
            *ppv = static_cast<ITfEditSession*>(this); AddRef(); return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++m_cRef; }
    STDMETHODIMP_(ULONG) Release() override { ULONG c = --m_cRef; if (!c) delete this; return c; }

    STDMETHODIMP DoEditSession(TfEditCookie ec) override {
        if (m_flush) {
            Str s = m_ts->engine().flush();
            if (m_ts->composition()) { SetText(m_ts, ec, m_pic, s); EndComposition(m_ts, ec); }
            else if (!s.empty())     { StartComposition(m_ts, ec, m_pic); SetText(m_ts, ec, m_pic, s); EndComposition(m_ts, ec); }
            return S_OK;
        }
        return ApplyInSession(m_ts, ec, m_pic, m_result);
    }
private:
    ~CEditSession() { m_pic->Release(); }
    LONG         m_cRef;
    TextService* m_ts;
    ITfContext*  m_pic;
    Result       m_result;
    bool         m_flush;
};

bool ModifierHeld() { // Ctrl / Alt / Win held -> not our keystroke
    auto down = [](int vk) { return (GetKeyState(vk) & 0x8000) != 0; };
    return down(VK_CONTROL) || down(VK_MENU) || down(VK_LWIN) || down(VK_RWIN);
}
unsigned ScanOf(LPARAM lParam) { return static_cast<unsigned>((lParam >> 16) & 0xFF); }
bool ShiftHeld() { return (GetKeyState(VK_SHIFT) & 0x8000) != 0; }

} // namespace

// ---------------------------------------------------------------------------
// TextService
// ---------------------------------------------------------------------------
TextService::TextService() : m_cRef(1) { DllAddRef(); }
TextService::~TextService() { DllRelease(); }

STDMETHODIMP TextService::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) return E_POINTER;
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfTextInputProcessor))
        *ppv = static_cast<ITfTextInputProcessor*>(this);
#if BK_HAS_TIP_EX
    else if (IsEqualIID(riid, IID_ITfTextInputProcessorEx))
        *ppv = static_cast<ITfTextInputProcessorEx*>(this);
#endif
    else if (IsEqualIID(riid, IID_ITfThreadMgrEventSink))
        *ppv = static_cast<ITfThreadMgrEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfKeyEventSink))
        *ppv = static_cast<ITfKeyEventSink*>(this);
    else if (IsEqualIID(riid, IID_ITfCompositionSink))
        *ppv = static_cast<ITfCompositionSink*>(this);
    else { *ppv = nullptr; return E_NOINTERFACE; }
    AddRef();
    return S_OK;
}
STDMETHODIMP_(ULONG) TextService::AddRef()  { return ++m_cRef; }
STDMETHODIMP_(ULONG) TextService::Release() { ULONG c = --m_cRef; if (!c) delete this; return c; }

HRESULT TextService::doActivate(ITfThreadMgr* tm, TfClientId id) {
    m_pThreadMgr = tm; m_pThreadMgr->AddRef();
    m_clientId = id;
    return advise(tm);
}

STDMETHODIMP TextService::Activate(ITfThreadMgr* tm, TfClientId id) { return doActivate(tm, id); }

#if BK_HAS_TIP_EX
STDMETHODIMP TextService::ActivateEx(ITfThreadMgr* tm, TfClientId id, DWORD /*flags*/) { return doActivate(tm, id); }
#endif

STDMETHODIMP TextService::Deactivate() {
    unadvise();
    if (m_pThreadMgr) { m_pThreadMgr->Release(); m_pThreadMgr = nullptr; }
    m_clientId = TF_CLIENTID_NULL;
    return S_OK;
}

HRESULT TextService::advise(ITfThreadMgr* tm) {
    // thread-manager event sink (focus changes)
    ITfSource* pSource = nullptr;
    if (SUCCEEDED(tm->QueryInterface(IID_ITfSource, (void**)&pSource))) {
        pSource->AdviseSink(IID_ITfThreadMgrEventSink, static_cast<ITfThreadMgrEventSink*>(this), &m_dwThreadMgrCookie);
        pSource->Release();
    }
    // key event sink
    ITfKeystrokeMgr* pKeic = nullptr;
    if (SUCCEEDED(tm->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeic))) {
        pKeic->AdviseKeyEventSink(m_clientId, static_cast<ITfKeyEventSink*>(this), TRUE);
        pKeic->Release();
    }
    return S_OK;
}

void TextService::unadvise() {
    if (!m_pThreadMgr) return;
    ITfKeystrokeMgr* pKeic = nullptr;
    if (SUCCEEDED(m_pThreadMgr->QueryInterface(IID_ITfKeystrokeMgr, (void**)&pKeic))) {
        pKeic->UnadviseKeyEventSink(m_clientId);
        pKeic->Release();
    }
    if (m_dwThreadMgrCookie != TF_INVALID_COOKIE) {
        ITfSource* pSource = nullptr;
        if (SUCCEEDED(m_pThreadMgr->QueryInterface(IID_ITfSource, (void**)&pSource))) {
            pSource->UnadviseSink(m_dwThreadMgrCookie);
            pSource->Release();
        }
        m_dwThreadMgrCookie = TF_INVALID_COOKIE;
    }
}

// ITfThreadMgrEventSink: flush composing text when focus leaves the document.
STDMETHODIMP TextService::OnSetFocus(ITfDocumentMgr* /*pNew*/, ITfDocumentMgr* pPrev) {
    if (pPrev) {
        ITfContext* pic = nullptr;
        if (SUCCEEDED(pPrev->GetTop(&pic)) && pic) { flushInto(pic); pic->Release(); }
    }
    return S_OK;
}

// ITfKeyEventSink
STDMETHODIMP TextService::OnSetFocus(BOOL /*fForeground*/) { return S_OK; }

STDMETHODIMP TextService::OnTestKeyDown(ITfContext* /*pic*/, WPARAM /*wParam*/, LPARAM lParam, BOOL* pEaten) {
    *pEaten = FALSE;
    if (ModifierHeld()) return S_OK;            // pass Ctrl/Alt/Win chords through
    if (m_engine.wouldHandle(ScanOf(lParam))) *pEaten = TRUE;
    return S_OK;
}

STDMETHODIMP TextService::OnKeyDown(ITfContext* pic, WPARAM /*wParam*/, LPARAM lParam, BOOL* pEaten) {
    *pEaten = FALSE;
    if (ModifierHeld()) { flushInto(pic); return S_OK; } // flush, let host handle the shortcut
    unsigned scan = ScanOf(lParam);
    if (!m_engine.wouldHandle(scan)) return S_OK;

    Result r = m_engine.process(scan, ShiftHeld());
    if (r.kind == ResultKind::Ignore) return S_OK;
    applyResult(pic, r);
    *pEaten = TRUE;
    return S_OK;
}

// ITfCompositionSink: the host ended our composition (e.g. mouse click elsewhere).
STDMETHODIMP TextService::OnCompositionTerminated(TfEditCookie /*ec*/, ITfComposition* pComposition) {
    if (pComposition == m_pComposition) {
        pComposition->Release();
        m_pComposition = nullptr;
        m_engine.reset();
    }
    return S_OK;
}

HRESULT TextService::applyResult(ITfContext* pic, const Result& r) {
    CEditSession* pSession = new (std::nothrow) CEditSession(this, pic, r);
    if (!pSession) return E_OUTOFMEMORY;
    HRESULT hrSession = S_OK;
    HRESULT hr = pic->RequestEditSession(m_clientId, pSession, TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
    pSession->Release();
    return SUCCEEDED(hr) ? hrSession : hr;
}

HRESULT TextService::flushInto(ITfContext* pic) {
    if (m_engine.empty() && !m_pComposition) return S_OK;
    CEditSession* pSession = new (std::nothrow) CEditSession(this, pic, true /*flush*/);
    if (!pSession) return E_OUTOFMEMORY;
    HRESULT hrSession = S_OK;
    HRESULT hr = pic->RequestEditSession(m_clientId, pSession, TF_ES_SYNC | TF_ES_READWRITE, &hrSession);
    pSession->Release();
    return SUCCEEDED(hr) ? hrSession : hr;
}
