// Bangla Keyboard — TSF text service core.
//
// Implements the TIP COM object. It owns one engine instance (../engine/) and
// turns engine Results into TSF composition operations:
//   Compose            -> start composition if needed, set the composition string
//   Commit             -> set string, end composition
//   CommitThenCompose  -> set `final`, end composition, start a new one with `comp`
//
// SCAFFOLD: structurally complete, but must be compiled & verified on a Windows
// SDK box. Spots needing SDK-time attention are tagged // VERIFY.
#pragma once

#include "Globals.h"
#include "../engine/engine.h"

class TextService final
    : public BK_TIP_BASE                 // ITfTextInputProcessorEx, or ...Processor on MinGW
    , public ITfThreadMgrEventSink
    , public ITfKeyEventSink
    , public ITfCompositionSink {
public:
    TextService();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ITfTextInputProcessor / Ex
    STDMETHODIMP Activate(ITfThreadMgr* tm, TfClientId id) override;
#if BK_HAS_TIP_EX
    STDMETHODIMP ActivateEx(ITfThreadMgr* tm, TfClientId id, DWORD flags) override;
#endif
    STDMETHODIMP Deactivate() override;

    // ITfThreadMgrEventSink (flush on focus changes)
    STDMETHODIMP OnInitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnUninitDocumentMgr(ITfDocumentMgr*) override { return S_OK; }
    STDMETHODIMP OnSetFocus(ITfDocumentMgr* pNew, ITfDocumentMgr* pPrev) override;
    STDMETHODIMP OnPushContext(ITfContext*) override { return S_OK; }
    STDMETHODIMP OnPopContext(ITfContext*) override { return S_OK; }

    // ITfKeyEventSink
    STDMETHODIMP OnSetFocus(BOOL fForeground) override;
    STDMETHODIMP OnTestKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pEaten) override;
    STDMETHODIMP OnKeyDown(ITfContext* pic, WPARAM wParam, LPARAM lParam, BOOL* pEaten) override;
    STDMETHODIMP OnTestKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pEaten) override { *pEaten = FALSE; return S_OK; }
    STDMETHODIMP OnKeyUp(ITfContext*, WPARAM, LPARAM, BOOL* pEaten) override { *pEaten = FALSE; return S_OK; }
    STDMETHODIMP OnPreservedKey(ITfContext*, REFGUID, BOOL* pEaten) override { *pEaten = FALSE; return S_OK; }

    // ITfCompositionSink
    STDMETHODIMP OnCompositionTerminated(TfEditCookie ecWrite, ITfComposition* pComposition) override;

    // --- helpers used by edit sessions (public so the session classes can call) -
    ITfComposition* composition() const { return m_pComposition; }
    void setComposition(ITfComposition* c) { m_pComposition = c; } // takes the ref the caller holds
    TfClientId clientId() const { return m_clientId; }
    bangla::Engine& engine() { return m_engine; }

    // Apply one engine Result inside an edit session on `pic`.
    HRESULT applyResult(ITfContext* pic, const bangla::Result& r);
    // Flush the engine (commit composing text as final) — e.g. on focus loss.
    HRESULT flushInto(ITfContext* pic);

private:
    ~TextService();

    HRESULT doActivate(ITfThreadMgr*, TfClientId); // shared by Activate / ActivateEx
    HRESULT advise(ITfThreadMgr*);
    void    unadvise();

    LONG            m_cRef;
    ITfThreadMgr*   m_pThreadMgr = nullptr;
    TfClientId      m_clientId   = TF_CLIENTID_NULL;
    DWORD           m_dwThreadMgrCookie = TF_INVALID_COOKIE;
    ITfComposition* m_pComposition = nullptr; // non-null while composing
    bangla::Engine  m_engine;
};
