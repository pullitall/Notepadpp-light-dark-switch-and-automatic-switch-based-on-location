#include "pch.h"
#include "PluginCommands.h"
#include <Windows.h>
#include <string>
#include <ctime>
#include <cmath>
#include <memory>
#include "PluginInterface.h"
#include "Notepad_plus_msgs.h"
#include "resource.h"

// Notepad++ Settings > Preferences menu command (menuCmdID.h: IDM_SETTING + 11, IDM_SETTING = 48000).
#define IDM_SETTING_PREFERENCE 48011

static const UINT AUTO_TIMER_INTERVAL_MS = 5 * 60 * 1000; // re-check every 5 minutes
static UINT_PTR g_autoTimer = 0;

// Forward declarations for internal helpers
static void loadSettings();
static void saveSettings();
static void computeSunriseSunset(int& srH, int& srM, int& ssH, int& ssM, bool& isDayNow);
static void applyMode(bool wantDark);
static void stopAutoTimer();
static void CALLBACK autoTimerProc(HWND, UINT, UINT_PTR, DWORD);

// Global settings
static double g_latitude = 52.0;
static double g_longitude = 19.0;
static int g_locationSet = 0;

struct CityInfo {
    const wchar_t* name;
    double lat;
    double lon;
};

static const CityInfo g_cities[] = {
    { L"Custom / Manual", 0, 0 },
    { L"Honolulu, USA (UTC-10)", 21.3069, -157.8583 },
    { L"Anchorage, USA (UTC-9)", 61.2181, -149.9003 },
    { L"Los Angeles, USA (UTC-8)", 34.0522, -118.2437 },
    { L"Vancouver, Canada (UTC-8)", 49.2827, -123.1207 },
    { L"Tijuana, Mexico (UTC-8)", 32.5149, -117.0382 },
    { L"Denver, USA (UTC-7)", 39.7392, -104.9903 },
    { L"Phoenix, USA (UTC-7)", 33.4484, -112.0740 },
    { L"Edmonton, Canada (UTC-7)", 53.5461, -113.4938 },
    { L"Chicago, USA (UTC-6)", 41.8781, -87.6298 },
    { L"Mexico City, Mexico (UTC-6)", 19.4326, -99.1332 },
    { L"Winnipeg, Canada (UTC-6)", 49.8951, -97.1384 },
    { L"New York, USA (UTC-5)", 40.7128, -74.0060 },
    { L"Washington DC, USA (UTC-5)", 38.9072, -77.0369 },
    { L"Toronto, Canada (UTC-5)", 43.6532, -79.3832 },
    { L"Lima, Peru (UTC-5)", -12.0464, -77.0428 },
    { L"Santiago, Chile (UTC-4)", -33.4489, -70.6693 },
    { L"Caracas, Venezuela (UTC-4)", 10.4806, -66.9036 },
    { L"Halifax, Canada (UTC-4)", 44.6488, -63.5752 },
    { L"Buenos Aires, Argentina (UTC-3)", -34.6037, -58.3816 },
    { L"Sao Paulo, Brazil (UTC-3)", -23.5505, -46.6333 },
    { L"Nuuk, Greenland (UTC-3)", 64.1750, -51.7389 },
    { L"Praia, Cape Verde (UTC-1)", 14.9177, -23.5092 },
    { L"Ponta Delgada, Azores (UTC-1)", 37.7412, -25.6756 },
    { L"London, UK (UTC+0)", 51.5074, -0.1278 },
    { L"Lisbon, Portugal (UTC+0)", 38.7223, -9.1393 },
    { L"Casablanca, Morocco (UTC+0)", 33.5731, -7.5898 },
    { L"Warsaw, Poland (UTC+1)", 52.2297, 21.0122 },
    { L"Berlin, Germany (UTC+1)", 52.5200, 13.4050 },
    { L"Paris, France (UTC+1)", 48.8566, 2.3522 },
    { L"Rome, Italy (UTC+1)", 41.9028, 12.4964 },
    { L"Lagos, Nigeria (UTC+1)", 6.5244, 3.3792 },
    { L"Athens, Greece (UTC+2)", 37.9838, 23.7275 },
    { L"Cairo, Egypt (UTC+2)", 30.0444, 31.2357 },
    { L"Helsinki, Finland (UTC+2)", 60.1699, 24.9384 },
    { L"Jerusalem, Israel (UTC+2)", 31.7683, 35.2137 },
    { L"Johannesburg, S.Africa (UTC+2)", -26.2041, 28.0473 },
    { L"Moscow, Russia (UTC+3)", 55.7558, 37.6173 },
    { L"Istanbul, Turkey (UTC+3)", 41.0082, 28.9784 },
    { L"Riyadh, Saudi Arabia (UTC+3)", 24.7136, 46.6753 },
    { L"Nairobi, Kenya (UTC+3)", -1.2921, 36.8219 },
    { L"Dubai, UAE (UTC+4)", 25.2048, 55.2708 },
    { L"Baku, Azerbaijan (UTC+4)", 40.4093, 49.8671 },
    { L"Karachi, Pakistan (UTC+5)", 24.8607, 67.0011 },
    { L"Tashkent, Uzbekistan (UTC+5)", 41.2995, 69.2401 },
    { L"New Delhi, India (UTC+5.5)", 28.6139, 77.2090 },
    { L"Mumbai, India (UTC+5.5)", 19.0760, 72.8777 },
    { L"Dhaka, Bangladesh (UTC+6)", 23.8103, 90.4125 },
    { L"Almaty, Kazakhstan (UTC+6)", 43.2220, 76.8512 },
    { L"Bangkok, Thailand (UTC+7)", 13.7563, 100.5018 },
    { L"Jakarta, Indonesia (UTC+7)", -6.2088, 106.8456 },
    { L"Beijing, China (UTC+8)", 39.9042, 116.4074 },
    { L"Singapore (UTC+8)", 1.3521, 103.8198 },
    { L"Perth, Australia (UTC+8)", -31.9505, 115.8605 },
    { L"Tokyo, Japan (UTC+9)", 35.6762, 139.6503 },
    { L"Seoul, South Korea (UTC+9)", 37.5665, 126.9780 },
    { L"Sydney, Australia (UTC+10)", -33.8688, 151.2093 },
    { L"Brisbane, Australia (UTC+10)", -27.4705, 153.0260 },
    { L"Vladivostok, Russia (UTC+10)", 43.1155, 131.8855 },
    { L"Auckland, New Zealand (UTC+12)", -36.8485, 174.7633 },
    { L"Suva, Fiji (UTC+12)", -18.1248, 178.4501 }
};

// --- PUBLIC INTERFACE FUNCTIONS ---

void checkFirstRun() {
    loadSettings();
    if (g_locationSet == 0) {
        if (MessageBox(nppData._nppHandle, L"Welcome to Dark Mode C!\nPlease set your location for Auto Mode.", L"Setup", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
            showSettingsDlg();
        }
    }
}

void switchToDayTheme()   { stopAutoTimer(); applyMode(false); }
void switchToNightTheme() { stopAutoTimer(); applyMode(true); }

void autoSwitchTheme() {
    loadSettings();
    int srH, srM, ssH, ssM; bool isDayNow;
    computeSunriseSunset(srH, srM, ssH, ssM, isDayNow);

    wchar_t msg[512];
    swprintf_s(msg, 512,
        L"Auto Mode active.\n\nLocation: %.2f, %.2f\nSunrise: %02d:%02d\nSunset: %02d:%02d\n\nStarting in %s mode. Will flip automatically.",
        g_latitude, g_longitude, srH, srM, ssH, ssM, isDayNow ? L"Day" : L"Night");
    MessageBox(nppData._nppHandle, msg, L"Auto Mode", MB_OK | MB_ICONINFORMATION);

    applyMode(!isDayNow);

    stopAutoTimer();
    g_autoTimer = SetTimer(NULL, 0, AUTO_TIMER_INTERVAL_MS, autoTimerProc);
}

void shutdownPlugin() { stopAutoTimer(); }

INT_PTR CALLBACK SettingsDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
    {
        ::SendMessage(nppData._nppHandle, NPPM_DARKMODESUBCLASSANDTHEME, (WPARAM)NppDarkMode::dmfInit, (LPARAM)hwnd);
        HWND hCombo = GetDlgItem(hwnd, IDC_CITY_COMBO);
        for (int i = 0; i < _countof(g_cities); ++i) SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)g_cities[i].name);
        SendMessage(hCombo, CB_SETCURSEL, 0, 0);
        wchar_t lat[32], lon[32]; swprintf_s(lat, 32, L"%.4f", g_latitude); swprintf_s(lon, 32, L"%.4f", g_longitude);
        SetDlgItemText(hwnd, IDC_LATITUDE, lat); SetDlgItemText(hwnd, IDC_LONGITUDE, lon);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            int idx = (int)SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
            if (idx > 0 && idx < _countof(g_cities)) {
                wchar_t lat[32], lon[32]; swprintf_s(lat, 32, L"%.4f", g_cities[idx].lat); swprintf_s(lon, 32, L"%.4f", g_cities[idx].lon);
                SetDlgItemText(hwnd, IDC_LATITUDE, lat); SetDlgItemText(hwnd, IDC_LONGITUDE, lon);
            }
            return TRUE;
        }
        if (LOWORD(wParam) == IDOK) {
            wchar_t lat[32], lon[32]; GetDlgItemText(hwnd, IDC_LATITUDE, lat, 32); GetDlgItemText(hwnd, IDC_LONGITUDE, lon, 32);
            g_latitude = _wtof(lat); g_longitude = _wtof(lon); g_locationSet = 1; saveSettings(); EndDialog(hwnd, IDOK); return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) { EndDialog(hwnd, IDCANCEL); return TRUE; }
        break;
    }
    return FALSE;
}

void showSettingsDlg() { DialogBox(hModule, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG), nppData._nppHandle, SettingsDlgProc); }

// --- INTERNAL HELPERS ---

// Notepad++ exposes no public API to flip dark mode at runtime. The only live
// path is through the Preferences dialog, which directly mutates NPP's internal
// state. We open it off-screen, select the Dark Mode tab, click the Light/Dark
// radio, and close — the UI flips without a restart.
//
// Language-independent: we locate the dialog by window class (#32770, the
// standard Win32 dialog class) and find the Dark Mode tab by probing for
// NPP's stable control IDs rather than matching UI text.

// From NPP source: PowerEditor/src/WinControls/Preference/preference_rc.h
#define NPP_IDC_RADIO_DARKMODE_LIGHTMODE      7131
#define NPP_IDC_RADIO_DARKMODE_DARKMODE       7132

struct FindByIdCtx {
    int wantId;
    HWND found;
};

static BOOL CALLBACK findChildByIdProc(HWND hwnd, LPARAM lp) {
    auto* ctx = reinterpret_cast<FindByIdCtx*>(lp);
    if (GetDlgCtrlID(hwnd) == ctx->wantId) {
        wchar_t cls[32];
        if (GetClassName(hwnd, cls, 32) && _wcsicmp(cls, L"Button") == 0) {
            ctx->found = hwnd;
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL CALLBACK findListBoxProc(HWND hwnd, LPARAM lp) {
    wchar_t cls[32];
    if (GetClassName(hwnd, cls, 32) && _wcsicmp(cls, L"ListBox") == 0) {
        *reinterpret_cast<HWND*>(lp) = hwnd;
        return FALSE;
    }
    return TRUE;
}

static BOOL CALLBACK findPrefsProc(HWND hwnd, LPARAM lp) {
    if (!IsWindowVisible(hwnd)) return TRUE;
    wchar_t cls[32];
    if (GetClassName(hwnd, cls, 32) == 0) return TRUE;
    // Preferences is a standard modal dialog (class #32770) owned by NPP's
    // main window. Filter on both so we don't grab an unrelated popup.
    if (_wcsicmp(cls, L"#32770") != 0) return TRUE;
    if (GetWindow(hwnd, GW_OWNER) != nppData._nppHandle) return TRUE;
    // Must contain a ListBox (the applet selector) to be Preferences.
    HWND lb = nullptr;
    EnumChildWindows(hwnd, findListBoxProc, reinterpret_cast<LPARAM>(&lb));
    if (!lb) return TRUE;
    *reinterpret_cast<HWND*>(lp) = hwnd;
    return FALSE;
}

struct ApplyModeArgs {
    bool wantDark;
    DWORD nppThreadId;
};

static DWORD WINAPI applyModeWorker(LPVOID lpParam) {
    std::unique_ptr<ApplyModeArgs> args(reinterpret_cast<ApplyModeArgs*>(lpParam));

    HWND prefs = nullptr;
    for (int i = 0; i < 200 && !prefs; ++i) {
        Sleep(10);
        EnumThreadWindows(args->nppThreadId, findPrefsProc, reinterpret_cast<LPARAM>(&prefs));
    }
    if (!prefs) return 1;

    // Park off-screen so the user never sees the dialog flash.
    SetWindowPos(prefs, nullptr, -32000, -32000, 0, 0,
                 SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    HWND listBox = nullptr;
    EnumChildWindows(prefs, findListBoxProc, reinterpret_cast<LPARAM>(&listBox));
    if (!listBox) { PostMessageW(prefs, WM_CLOSE, 0, 0); return 2; }

    // Iterate applets by index instead of by UI text (which varies by NPP
    // language). Each selection triggers LBN_SELCHANGE, which makes NPP
    // lazy-create the corresponding sub-dialog. The Dark Mode applet is the
    // one whose sub-dialog exposes NPP's dark-mode radio IDs.
    int count = static_cast<int>(SendMessageW(listBox, LB_GETCOUNT, 0, 0));
    if (count <= 0) { PostMessageW(prefs, WM_CLOSE, 0, 0); return 3; }
    int listId = GetDlgCtrlID(listBox);
    HWND parent = GetParent(listBox);

    HWND radio = nullptr;
    for (int i = 0; i < count && !radio; ++i) {
        SendMessageW(listBox, LB_SETCURSEL, i, 0);
        SendMessageW(parent, WM_COMMAND,
            MAKEWPARAM(listId, LBN_SELCHANGE), reinterpret_cast<LPARAM>(listBox));
        // Sub-dialog creation is synchronous for LBN_SELCHANGE, but give
        // Windows a tick to finish child-window wiring before probing.
        Sleep(20);
        int wantId = args->wantDark
            ? NPP_IDC_RADIO_DARKMODE_DARKMODE
            : NPP_IDC_RADIO_DARKMODE_LIGHTMODE;
        FindByIdCtx ctx = { wantId, nullptr };
        EnumChildWindows(prefs, findChildByIdProc, reinterpret_cast<LPARAM>(&ctx));
        if (ctx.found) radio = ctx.found;
    }
    if (!radio) { PostMessageW(prefs, WM_CLOSE, 0, 0); return 4; }

    // BM_CLICK simulates a real click: triggers BS_AUTORADIOBUTTON group
    // behavior (unchecks siblings) AND posts WM_COMMAND(BN_CLICKED) to parent.
    // BM_SETCHECK alone is not enough — NPP reads the Dark radio's state to
    // decide whether dark mode is on, so the sibling must be unchecked too.
    SendMessageW(radio, BM_CLICK, 0, 0);

    Sleep(80);
    PostMessageW(prefs, WM_CLOSE, 0, 0);
    return 0;
}

static void applyMode(bool wantDark) {
    bool nppIsDark = (::SendMessage(nppData._nppHandle, NPPM_ISDARKMODEENABLED, 0, 0) != 0);
    if (nppIsDark == wantDark) return;

    auto* args = new ApplyModeArgs{ wantDark, GetWindowThreadProcessId(nppData._nppHandle, nullptr) };
    HANDLE h = CreateThread(nullptr, 0, applyModeWorker, args, 0, nullptr);
    if (!h) { delete args; return; }
    CloseHandle(h);

    // Blocks until the worker posts WM_CLOSE on Preferences.
    ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_SETTING_PREFERENCE);
}

static void computeSunriseSunset(int& srH, int& srM, int& ssH, int& ssM, bool& isDayNow) {
    time_t now = time(nullptr); struct tm lt; localtime_s(&lt, &now);
    double decl = 0.409 * sin(0.0172 * (lt.tm_yday + 1 - 80));
    double latR = g_latitude * 0.0174533; double cosH = -tan(latR) * tan(decl);
    if (cosH > 1.0) cosH = 1.0; if (cosH < -1.0) cosH = -1.0;
    double hDay = 12.0 * acos(cosH) / 3.14159;
    double lonCorr = (g_longitude - 15.0) * (4.0 / 60.0);
    double noon = (lt.tm_isdst > 0) ? (13.0 - lonCorr) : (12.0 - lonCorr);
    double srD = noon - hDay; double ssD = noon + hDay;
    srH = (int)srD; srM = (int)((srD - srH) * 60);
    ssH = (int)ssD; ssM = (int)((ssD - ssH) * 60);
    double nowD = lt.tm_hour + lt.tm_min / 60.0;
    isDayNow = (nowD >= srD && nowD < ssD);
}

static void stopAutoTimer() {
    if (g_autoTimer) { KillTimer(NULL, g_autoTimer); g_autoTimer = 0; }
}

static void CALLBACK autoTimerProc(HWND, UINT, UINT_PTR, DWORD) {
    int srH, srM, ssH, ssM; bool isDayNow;
    computeSunriseSunset(srH, srM, ssH, ssM, isDayNow);
    applyMode(!isDayNow);
}

static std::wstring getIniPath() {
    wchar_t path[MAX_PATH]; ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)path);
    std::wstring iniPath = path; if (iniPath.empty()) return L""; return iniPath + L"\\dark_mode_C.ini";
}

static void loadSettings() {
    std::wstring path = getIniPath(); if (path.empty()) return;
    wchar_t val[32]; GetPrivateProfileString(L"Location", L"Latitude", L"52.0", val, 32, path.c_str()); g_latitude = _wtof(val);
    GetPrivateProfileString(L"Location", L"Longitude", L"19.0", val, 32, path.c_str()); g_longitude = _wtof(val);
    g_locationSet = GetPrivateProfileInt(L"Location", L"LocationSet", 0, path.c_str());
}

static void saveSettings() {
    std::wstring path = getIniPath(); if (path.empty()) return;
    wchar_t lat[32], lon[32], locSet[10];
    swprintf_s(lat, 32, L"%.4f", g_latitude); swprintf_s(lon, 32, L"%.4f", g_longitude); swprintf_s(locSet, 10, L"%d", g_locationSet);
    WritePrivateProfileString(L"Location", L"Latitude", lat, path.c_str());
    WritePrivateProfileString(L"Location", L"Longitude", lon, path.c_str());
    WritePrivateProfileString(L"Location", L"LocationSet", locSet, path.c_str());
}
