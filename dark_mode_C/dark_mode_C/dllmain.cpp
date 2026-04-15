// Dllmain.cpp
#include "pch.h"
#include <windows.h>
#include <stdio.h>

extern HINSTANCE hModule;

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        hModule = hMod;  // Store the plugin DLL's module handle
#ifdef _DEBUG
        wchar_t msg[256];
        swprintf_s(msg, 256, L"DllMain: DLL_PROCESS_ATTACH, hModule=%p\n", hMod);
        OutputDebugString(msg);
#endif
    }
    return TRUE;
}
