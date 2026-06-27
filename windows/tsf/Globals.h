// Bangla Keyboard — TSF text service: shared GUIDs, IDs and constants.
//
// SCAFFOLD: this folder is the Windows TSF TIP that hosts the (verified) engine
// in ../engine/. It is structured to open in Visual Studio with the Windows SDK.
// COM/registration code here is standard TSF boilerplate and must be compiled &
// verified on a Windows SDK box (no compiler on the dev box where it was drafted).
#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601   // Windows 7+ (for RegDeleteTreeW etc.)
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <msctf.h>
#include <ole2.h>

// Older SDK/MinGW headers may lack this constant.
#ifndef TF_CLIENTID_NULL
#define TF_CLIENTID_NULL ((TfClientId)0)
#endif

// ITfTextInputProcessorEx is a newer interface; MinGW's msctf.h doesn't define
// it. Use it when present (full SDK), else fall back to ITfTextInputProcessor.
#ifdef __ITfTextInputProcessorEx_INTERFACE_DEFINED__
#define BK_HAS_TIP_EX 1
#define BK_TIP_BASE ITfTextInputProcessorEx
#else
#define BK_HAS_TIP_EX 0
#define BK_TIP_BASE ITfTextInputProcessor
#endif

// --- identity (freshly generated; keep stable once shipped) -------------------
// TIP class object.
// {C4720D47-5015-4041-886F-54A92AD69BE2}
extern const CLSID c_clsidBanglaTextService;
// Language profile for this service.
// {460DB25E-A6DC-4978-8F3E-3B37AA1152A0}
extern const GUID  c_guidBanglaProfile;
// Display attribute (underline) for composing text.
// {BE758843-2F63-46B1-ABCC-6A7CDEB7F13E}
extern const GUID  c_guidDisplayAttribute;

// bn-BD. Bengali primary lang = 0x45; sublang Bangladesh = 0x02 -> LANGID 0x0845.
// (bn-IN would be 0x0445.) TSF profiles are keyed by LANGID + profile GUID.
constexpr LANGID kBanglaLangId = MAKELANGID(0x45, 0x02); // 0x0845

extern const WCHAR c_szServiceDescription[]; // shown in the language bar
extern const WCHAR c_szProfileDescription[];

extern HINSTANCE g_hInst;        // set in DllMain
extern LONG      g_cRefDll;      // module ref count (DllCanUnloadNow)

inline void DllAddRef()  { InterlockedIncrement(&g_cRefDll); }
inline void DllRelease() { InterlockedDecrement(&g_cRefDll); }
