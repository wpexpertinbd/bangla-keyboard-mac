// Non-destructive smoke test for BanglaKeyboard.dll: LoadLibrary + DllGetClassObject
// + CreateInstance + QueryInterface for the TSF sink interfaces. Touches NO registry
// (does not register the IME). Proves the COM object and its vtables are wired.
//
// Build (from windows/tsf/, with the DLL already built in ../dist):
//   g++ -std=c++17 -static loadtest.cpp -o ../dist/loadtest.exe -lole32 -luuid
//   (cd ../dist && ./loadtest.exe)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <msctf.h>
#include <cstdio>

// {C4720D47-5015-4041-886F-54A92AD69BE2}
static const CLSID kClsid = {0xC4720D47,0x5015,0x4041,{0x88,0x6F,0x54,0xA9,0x2A,0xD6,0x9B,0xE2}};
typedef HRESULT (STDAPICALLTYPE *PFN_GCO)(REFCLSID, REFIID, void**);
typedef HRESULT (STDAPICALLTYPE *PFN_CANUNLOAD)();

static void check(const char* what, HRESULT hr, void* p) {
    std::printf("  %-42s hr=0x%08lX  ptr=%p  %s\n", what, (unsigned long)hr, p,
                (SUCCEEDED(hr) && p) ? "OK" : "FAIL");
}

int main() {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    int rc = 0;

    HMODULE h = LoadLibraryW(L"BanglaKeyboard.dll");
    if (!h) { std::printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }
    std::printf("LoadLibrary BanglaKeyboard.dll -> %p\n", (void*)h);

    auto gco = (PFN_GCO)GetProcAddress(h, "DllGetClassObject");
    auto canUnload = (PFN_CANUNLOAD)GetProcAddress(h, "DllCanUnloadNow");
    if (!gco || !canUnload) { std::printf("missing exports\n"); return 1; }

    IClassFactory* pcf = nullptr;
    HRESULT hr = gco(kClsid, IID_IClassFactory, (void**)&pcf);
    check("DllGetClassObject(IID_IClassFactory)", hr, pcf);
    if (FAILED(hr) || !pcf) return 1;

    IUnknown* pTip = nullptr;
    hr = pcf->CreateInstance(nullptr, IID_ITfTextInputProcessor, (void**)&pTip);
    check("CreateInstance(ITfTextInputProcessor)", hr, pTip);
    if (SUCCEEDED(hr) && pTip) {
        void* p = nullptr;
        HRESULT q;
        q = pTip->QueryInterface(IID_ITfKeyEventSink, &p);       check("QI ITfKeyEventSink", q, p);       if (p) ((IUnknown*)p)->Release(); else rc = 1;
        q = pTip->QueryInterface(IID_ITfThreadMgrEventSink, &p); check("QI ITfThreadMgrEventSink", q, p); if (p) ((IUnknown*)p)->Release(); else rc = 1;
        q = pTip->QueryInterface(IID_ITfCompositionSink, &p);    check("QI ITfCompositionSink", q, p);    if (p) ((IUnknown*)p)->Release(); else rc = 1;
        pTip->Release();
    } else rc = 1;
    pcf->Release();

    std::printf("DllCanUnloadNow -> 0x%08lX (S_OK = no live objects)\n", (unsigned long)canUnload());
    FreeLibrary(h);
    CoUninitialize();
    std::printf(rc == 0 ? "\nLOADTEST PASS\n" : "\nLOADTEST FAIL\n");
    return rc;
}
