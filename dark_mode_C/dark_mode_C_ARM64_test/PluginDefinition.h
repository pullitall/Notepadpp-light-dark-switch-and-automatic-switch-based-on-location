#pragma once

#include <windows.h>
#include <tchar.h>
#include "Notepad_plus_msgs.h"  // For LangType and toolbarIconsWithDarkMode

// Define IconResourceMap struct for this plugin
struct IconResourceMap {
    int bmpId;
    int iconLightId;
    int iconDarkModeId;
};

// External function declarations
extern void switchToDayTheme();
extern void switchToNightTheme();
extern void autoSwitchTheme();
