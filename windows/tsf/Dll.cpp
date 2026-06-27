// Bangla Keyboard — DLL entry, class factory, and TSF registration.
#include "Globals.h"
#include "TextService.h"
#include <strsafe.h>
#include <new>

// ---------------------------------------------------------------------------
// Class factory
// ---------------------------------------------------------------------------
class ClassFactory final : public IClassFactory {
public:
    ClassFactory() : m_cRef(1) { DllAddRef(); }

    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory)) {
            *ppv = static_cast<IClassFactory*>(this); AddRef(); return S_OK;
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() override { return ++m_cRef; }
    STDMETHODIMP_(ULONG) Release() override { ULONG c = --m_cRef; if (!c) delete this; return c; }

    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override {
        if (pUnkOuter) return CLASS_E_NOAGGREGATION;
        TextService* p = new (std::nothrow) TextService();
        if (!p) return E_OUTOFMEMORY;
        HRESULT hr = p->QueryInterface(riid, ppv);
        p->Release();
        return hr;
    }
    STDMETHODIMP LockServer(BOOL fLock) override { if (fLock) DllAddRef(); else DllRelease(); return S_OK; }

private:
    ~ClassFactory() { DllRelease(); }
    LONG m_cRef;
};

// ---------------------------------------------------------------------------
// DLL exports
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
    switch (reason) {
        case DLL_PROCESS_ATTACH: g_hInst = hInst; DisableThreadLibraryCalls(hInst); break;
        default: break;
    }
    return TRUE;
}

STDAPI DllCanUnloadNow() { return g_cRefDll <= 0 ? S_OK : S_FALSE; }

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    if (!IsEqualCLSID(rclsid, c_clsidBanglaTextService)) return CLASS_E_CLASSNOTAVAILABLE;
    ClassFactory* pFactory = new (std::nothrow) ClassFactory();
    if (!pFactory) return E_OUTOFMEMORY;
    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    return hr;
}

// ---------------------------------------------------------------------------
// COM in-proc server registry entries (HKCR\CLSID\{clsid}\InprocServer32)
// ---------------------------------------------------------------------------
static HRESULT RegisterServer() {
    WCHAR szModule[MAX_PATH];
    if (!GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule))) return E_FAIL;

    WCHAR szClsid[64]; StringFromGUID2(c_clsidBanglaTextService, szClsid, ARRAYSIZE(szClsid));
    WCHAR szKey[128];
    StringCchPrintfW(szKey, ARRAYSIZE(szKey), L"CLSID\\%s", szClsid);

    HKEY hKey = nullptr, hSub = nullptr;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, szKey, 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) != ERROR_SUCCESS)
        return E_FAIL;
    RegSetValueExW(hKey, nullptr, 0, REG_SZ, (const BYTE*)c_szServiceDescription,
                   (DWORD)((wcslen(c_szServiceDescription) + 1) * sizeof(WCHAR)));
    if (RegCreateKeyExW(hKey, L"InprocServer32", 0, nullptr, 0, KEY_WRITE, nullptr, &hSub, nullptr) == ERROR_SUCCESS) {
        RegSetValueExW(hSub, nullptr, 0, REG_SZ, (const BYTE*)szModule, (DWORD)((wcslen(szModule) + 1) * sizeof(WCHAR)));
        const WCHAR* model = L"Apartment";
        RegSetValueExW(hSub, L"ThreadingModel", 0, REG_SZ, (const BYTE*)model, (DWORD)((wcslen(model) + 1) * sizeof(WCHAR)));
        RegCloseKey(hSub);
    }
    RegCloseKey(hKey);
    return S_OK;
}

static void UnregisterServer() {
    WCHAR szClsid[64]; StringFromGUID2(c_clsidBanglaTextService, szClsid, ARRAYSIZE(szClsid));
    WCHAR szKey[128]; StringCchPrintfW(szKey, ARRAYSIZE(szKey), L"CLSID\\%s", szClsid);
    RegDeleteTreeW(HKEY_CLASSES_ROOT, szKey);
}

// ---------------------------------------------------------------------------
// TSF profile + category registration
// ---------------------------------------------------------------------------
// MinGW's msctf.h lacks these TIP-capability category GUIDs (the full Windows
// SDK declares & defines them). Provide them here for the MinGW build only.
#ifdef __MINGW32__
EXTERN_C const GUID GUID_TFCAT_TIPCAP_SECUREMODE =
    {0x49d2f9ce,0x1f5e,0x11d7,{0xa6,0xd3,0x00,0x06,0x5b,0x84,0x43,0x5c}};
EXTERN_C const GUID GUID_TFCAT_TIPCAP_UIELEMENTENABLED =
    {0x49d2f9cf,0x1f5e,0x11d7,{0xa6,0xd3,0x00,0x06,0x5b,0x84,0x43,0x5c}};
EXTERN_C const GUID GUID_TFCAT_TIPCAP_COMLESS =
    {0x364215d9,0x75bc,0x11d7,{0xa6,0xef,0x00,0x06,0x5b,0x84,0x43,0x5c}};
#endif

static const GUID* kCategories[] = {
    &GUID_TFCAT_TIP_KEYBOARD,                 // it's a keyboard TIP
    &GUID_TFCAT_TIPCAP_SECUREMODE,            // works on secure desktops (UAC/login)
    &GUID_TFCAT_TIPCAP_UIELEMENTENABLED,
    &GUID_TFCAT_TIPCAP_COMLESS,               // can load without a thread CoInitialize quirk
    // &GUID_TFCAT_DISPLAYATTRIBUTEPROVIDER,  // add once the display-attribute provider exists
};

static HRESULT RegisterProfile() {
    WCHAR szModule[MAX_PATH];
    GetModuleFileNameW(g_hInst, szModule, ARRAYSIZE(szModule));

    ITfInputProcessorProfiles* pProfiles = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_ITfInputProcessorProfiles, (void**)&pProfiles);
    if (FAILED(hr)) return hr;

    hr = pProfiles->Register(c_clsidBanglaTextService);
    if (SUCCEEDED(hr)) {
        hr = pProfiles->AddLanguageProfile(
            c_clsidBanglaTextService, kBanglaLangId, c_guidBanglaProfile,
            c_szProfileDescription, (ULONG)wcslen(c_szProfileDescription),
            szModule, (ULONG)-1 /*icon index; -1 = default*/, 0);
    }
    pProfiles->Release();

    if (SUCCEEDED(hr)) {
        ITfCategoryMgr* pCat = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_ITfCategoryMgr, (void**)&pCat))) {
            for (const GUID* g : kCategories)
                pCat->RegisterCategory(c_clsidBanglaTextService, *g, c_clsidBanglaTextService);
            pCat->Release();
        }
    }
    return hr;
}

static void UnregisterProfile() {
    ITfInputProcessorProfiles* pProfiles = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_InputProcessorProfiles, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfInputProcessorProfiles, (void**)&pProfiles))) {
        pProfiles->Unregister(c_clsidBanglaTextService);
        pProfiles->Release();
    }
    ITfCategoryMgr* pCat = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_TF_CategoryMgr, nullptr, CLSCTX_INPROC_SERVER,
                                   IID_ITfCategoryMgr, (void**)&pCat))) {
        for (const GUID* g : kCategories)
            pCat->UnregisterCategory(c_clsidBanglaTextService, *g, c_clsidBanglaTextService);
        pCat->Release();
    }
}

// regsvr32 entry points
STDAPI DllRegisterServer() {
    HRESULT hr = RegisterServer();
    if (SUCCEEDED(hr)) hr = RegisterProfile();
    if (FAILED(hr)) { UnregisterProfile(); UnregisterServer(); }
    return hr;
}

STDAPI DllUnregisterServer() {
    UnregisterProfile();
    UnregisterServer();
    return S_OK;
}
