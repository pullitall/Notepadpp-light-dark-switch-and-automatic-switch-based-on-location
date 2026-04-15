#pragma once

#include <Windows.h>

#define NB_FUNC 4

typedef void (*PFUNCPLUGINCMD)();

struct FuncItem {
    TCHAR _itemName[64];
    PFUNCPLUGINCMD _pFunc;
    int _cmdID;
    BOOL _init2Check;
    void* _pShKey;
};

struct NppData {
    HWND _nppHandle;
    HWND _scintillaMainHandle;
    HWND _scintillaSecondHandle;
};

// Required plugin functions
void setCommand(size_t index, const TCHAR* cmdName, PFUNCPLUGINCMD pFunc, BOOL checkOnInit);

extern FuncItem funcItem[NB_FUNC];
extern NppData nppData;
extern HINSTANCE hModule;
