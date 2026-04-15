#include "pch.h"
#include "PluginDefinition.h"
#include "PluginCommands.h"
#include "PluginInterface.h"
#include "resource.h"
#include "Notepad_plus_msgs.h"
#include "Scintilla.h"
#include <windows.h>
#include <tchar.h>

// Global instance handle for the DLL, set in DllMain
HINSTANCE hModule = nullptr;

// FuncItem array to hold plugin command definitions
FuncItem funcItem[NB_FUNC] = {};  // Explicitly zero-initialize

// Notepad++ data structure
NppData nppData = {};  // Explicitly zero-initialize

// Function to initialize the plugin
void pluginInit() {
    // Reserved for future initialization (currently not needed)
}

// Helper function to set up a plugin command
void setCommand(size_t index, const TCHAR* cmdName, PFUNCPLUGINCMD pFunc, BOOL checkOnInit) {
    // Bounds check to prevent buffer overflow
    if (index >= _countof(funcItem)) {
        OutputDebugString(TEXT("ERROR: setCommand index out of bounds\n"));
        return;
    }
    if (!cmdName) {
        OutputDebugString(TEXT("ERROR: setCommand cmdName is NULL\n"));
        return;
    }
    if (!pFunc) {
        OutputDebugString(TEXT("ERROR: setCommand pFunc is NULL\n"));
        return;
    }

    _tcscpy_s(funcItem[index]._itemName, _countof(funcItem[index]._itemName), cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = checkOnInit;
    funcItem[index]._pShKey = nullptr;
    funcItem[index]._cmdID = 0;  // Notepad++ will auto-assign command IDs

#ifdef _DEBUG
    wchar_t msg[256];
    swprintf_s(msg, 256, L"setCommand[%zu]: %s, pFunc=%p\n", index, cmdName, pFunc);
    OutputDebugString(msg);
#endif
}

// Command menu initialization: sets up menu entries (Notepad++ will auto-assign command IDs)
void commandMenuInit() {
    static bool initialized = false;

#ifdef _DEBUG
    OutputDebugString(TEXT("commandMenuInit called\n"));
#endif

    if (initialized) {
#ifdef _DEBUG
        OutputDebugString(TEXT("commandMenuInit: already initialized, returning\n"));
#endif
        return;  // Prevent re-initialization
    }

    // Zero-initialize funcItem array to avoid garbage values
    ZeroMemory(funcItem, sizeof(funcItem));

#ifdef _DEBUG
    OutputDebugString(TEXT("commandMenuInit: Setting up commands...\n"));
#endif

    // Register menu commands.
    // Leave _cmdID as 0 - Notepad++ will automatically assign command IDs.
    setCommand(0, TEXT("Day Mode"), switchToDayTheme, FALSE);
    setCommand(1, TEXT("Night Mode"), switchToNightTheme, FALSE);
    setCommand(2, TEXT("Auto Mode"), autoSwitchTheme, FALSE);
    setCommand(3, TEXT("Settings"), showSettingsDlg, FALSE);

    initialized = true;

#ifdef _DEBUG
    OutputDebugString(TEXT("commandMenuInit: Complete\n"));
#endif
}

// Function to add toolbar icons to Notepad++
void addToolbarIcons() {
    static bool iconAdded[3] = { false, false, false };

#ifdef _DEBUG
    wchar_t msg[256];
    swprintf_s(msg, 256, L"addToolbarIcons called, hModule=%p, nppData._nppHandle=%p\n", hModule, nppData._nppHandle);
    OutputDebugString(msg);
#endif

    // Validate that DLL module handle and Notepad++ window handle are valid
    if (!hModule || !nppData._nppHandle) {
#ifdef _DEBUG
        OutputDebugString(TEXT("ERROR: addToolbarIcons - hModule or nppData._nppHandle is NULL\n"));
#endif
        return;
    }

    // Define the resource IDs for each command's icons
    IconResourceMap iconMaps[3] = {
        { IDB_BMP_SUN_16X16, IDI_ICON_SUN, IDI_ICON_SUN },       // Day Mode
        { IDB_BMP_NIGHT_16X16, IDI_ICON_NIGHT, IDI_ICON_NIGHT }, // Night Mode
        { IDB_BMP_AUTO_16X16, IDI_ICON_AUTO, IDI_ICON_AUTO }     // Auto Mode
    };

    // Only 3 icons exist in the map (Day, Night, Auto)
    for (int i = 0; i < 3; ++i) {
        if (iconAdded[i]) continue; // Already added

#ifdef _DEBUG
        wchar_t msg[256];
        swprintf_s(msg, 256, L"Processing icon for command %d, cmdID=%d\n", i, funcItem[i]._cmdID);
        OutputDebugString(msg);
#endif

        // Skip if command ID hasn't been assigned by Notepad++ yet
        if (funcItem[i]._cmdID == 0) {
#ifdef _DEBUG
            OutputDebugString(TEXT("WARNING: Command ID is 0, skipping this icon for now\n"));
#endif
            continue;
        }

        toolbarIconsWithDarkMode tb = {};  // Zero-initialize the struct

        // Load the 16x16 BMP for standard toolbar (hToolbarBmp)
        tb.hToolbarBmp = (HBITMAP)LoadImage(hModule, MAKEINTRESOURCE(iconMaps[i].bmpId), IMAGE_BITMAP, 16, 16, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);

        // Load the ICO for Fluent UI light mode (hToolbarIcon)
        tb.hToolbarIcon = (HICON)LoadImage(hModule, MAKEINTRESOURCE(iconMaps[i].iconLightId), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

        // Load the ICO for Fluent UI dark mode (hToolbarIconDarkMode)
        tb.hToolbarIconDarkMode = (HICON)LoadImage(hModule, MAKEINTRESOURCE(iconMaps[i].iconDarkModeId), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

#ifdef _DEBUG
        swprintf_s(msg, 256, L"Icon %d loaded: BMP=%p, ICO=%p, ICO_DM=%p\n",
            i, tb.hToolbarBmp, tb.hToolbarIcon, tb.hToolbarIconDarkMode);
        OutputDebugString(msg);
#endif

        // Validate all three handles before sending the message to Notepad++
        if (tb.hToolbarBmp && tb.hToolbarIcon && tb.hToolbarIconDarkMode) {
            LRESULT result = ::SendMessage(nppData._nppHandle, NPPM_ADDTOOLBARICON_FORDARKMODE, funcItem[i]._cmdID, (LPARAM)&tb);

#ifdef _DEBUG
            swprintf_s(msg, 256, L"SendMessage result for icon %d: %lld\n", i, (long long)result);
            OutputDebugString(msg);
#endif

            // If SendMessage failed, clean up the resources ourselves
            // Otherwise, Notepad++ takes ownership and will clean them up
            if (!result) {
#ifdef _DEBUG
                OutputDebugString(TEXT("SendMessage failed, cleaning up resources\n"));
#endif
                DeleteObject(tb.hToolbarBmp);
                DestroyIcon(tb.hToolbarIcon);
                DestroyIcon(tb.hToolbarIconDarkMode);
            } else {
                // Success! Mark as added so we don't try again.
                iconAdded[i] = true;
            }
        }
        else {
#ifdef _DEBUG
            OutputDebugString(TEXT("ERROR: One or more icon resources failed to load\n"));
#endif
            // Clean up resources if loading fails
            if (tb.hToolbarBmp) DeleteObject(tb.hToolbarBmp);
            if (tb.hToolbarIcon) DestroyIcon(tb.hToolbarIcon);
            if (tb.hToolbarIconDarkMode) DestroyIcon(tb.hToolbarIconDarkMode);
        }
    }
}

// Plugin cleanup function (called on NPPN_SHUTDOWN)
void commandMenuCleanUp() {
    // Notepad++ takes ownership of the icon handles passed via NPPM_ADDTOOLBARICON_FORDARKMODE
    // Therefore, we do not need to destroy them here.
}

// Exported functions for Notepad++ to interact with the plugin
extern "C" {

    // Called by Notepad++ to set plugin info
    __declspec(dllexport) void setInfo(NppData notpadPlusData) {
#ifdef _DEBUG
        wchar_t msg[256];
        swprintf_s(msg, 256, L"setInfo called, nppHandle=%p\n", notpadPlusData._nppHandle);
        OutputDebugString(msg);
#endif
        nppData = notpadPlusData;
        pluginInit();
    }

    // Called by Notepad++ to get plugin name
    __declspec(dllexport) const TCHAR* getName() {
#ifdef _DEBUG
        OutputDebugString(TEXT("getName called\n"));
#endif
        return TEXT("dark_mode_C");
    }

    // Called by Notepad++ to get plugin commands array
    __declspec(dllexport) FuncItem* getFuncsArray(int* pnbFunc) {
#ifdef _DEBUG
        OutputDebugString(TEXT("getFuncsArray called\n"));
#endif
        if (!pnbFunc) {
#ifdef _DEBUG
            OutputDebugString(TEXT("ERROR: getFuncsArray - pnbFunc is NULL\n"));
#endif
            return nullptr;  // Safety check
        }

        commandMenuInit();
        *pnbFunc = NB_FUNC;  // Must match the size of funcItem array and number of setCommand calls

#ifdef _DEBUG
        for (int i = 0; i < NB_FUNC; i++) {
            wchar_t msg[256];
            swprintf_s(msg, 256, L"funcItem[%d]: name='%s', pFunc=%p, cmdID=%d\n",
                i, funcItem[i]._itemName, funcItem[i]._pFunc, funcItem[i]._cmdID);
            OutputDebugString(msg);
        }
#endif
        return funcItem;
    }

    // Called by Notepad++ to check if plugin supports Unicode
    __declspec(dllexport) BOOL isUnicode() {
        return TRUE;
    }

    // Message processing function (for custom messages if needed)
    __declspec(dllexport) LRESULT messageProc(UINT msg, WPARAM wParam, LPARAM lParam) {
        return 0;
    }

    // Notification handler for Notepad++ events
    __declspec(dllexport) void beNotified(SCNotification* notification) {
        if (!notification) return;  // Safety check

#ifdef _DEBUG
        wchar_t msg[256];
        swprintf_s(msg, 256, L"beNotified called, code=%d\n", notification->nmhdr.code);
        OutputDebugString(msg);
#endif

        switch (notification->nmhdr.code) {
        case NPPN_TBMODIFICATION:
#ifdef _DEBUG
            OutputDebugString(TEXT("Received NPPN_TBMODIFICATION\n"));
#endif
            addToolbarIcons();
            break;

        case NPPN_READY:
#ifdef _DEBUG
            OutputDebugString(TEXT("Received NPPN_READY\n"));
#endif
            addToolbarIcons();
            checkFirstRun(); // Trigger the setup reminder
            break;

        case NPPN_SHUTDOWN:
#ifdef _DEBUG
            OutputDebugString(TEXT("Received NPPN_SHUTDOWN\n"));
#endif
            shutdownPlugin();
            commandMenuCleanUp();
            break;
        }
    }
}
