// ============================================================================
// EliteStartMenu.cpp - Native C++ Start Menu for Elite-TaskBar
// Phase 4: Replaces the old PS2EXE-compiled PowerShell start menu
//
// Copyright (C) 2026 EliteSoftwareTech Co. All rights reserved.
// ============================================================================

#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "ole32.lib")

// ============================================================================
// Forward Declarations
// ============================================================================
static LRESULT CALLBACK IPCWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK StartMenuWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void ShowStartMenu(int monitorIndex, int taskbarY);
static void HideStartMenu();
static void ToggleStartMenu();
static void LaunchSettings();

// ============================================================================
// Globals
// ============================================================================
static HINSTANCE g_hInstance      = NULL;
static HWND      g_hIPCWnd       = NULL;     // Hidden message-only window for IPC
static HWND      g_hMenuWnd      = NULL;     // The visible popup start menu window
static UINT      g_WM_TOGGLE     = 0;        // Registered window message for IPC
static HANDLE    g_hMutex        = NULL;      // Single-instance mutex
static bool      g_bMenuVisible  = false;
static int       g_monitorIndex  = 0;
static int       g_taskbarY      = 0;

// Menu dimensions
static const int MENU_WIDTH  = 385;
static const int MENU_HEIGHT = 520;

// Color palette (Win32 classic aesthetic)
static const COLORREF CLR_MENU_BG       = RGB(240, 240, 240);
static const COLORREF CLR_SIDEBAR_BG    = RGB(70, 130, 180);  // Steel blue sidebar
static const COLORREF CLR_SEPARATOR     = RGB(200, 200, 200);
static const COLORREF CLR_HOVER_BG      = RGB(209, 228, 245);
static const COLORREF CLR_TEXT_NORMAL   = RGB(0, 0, 0);
static const COLORREF CLR_TEXT_SIDEBAR  = RGB(255, 255, 255);
static const COLORREF CLR_SEARCH_BORDER = RGB(122, 122, 122);

// Child control IDs
#define IDC_SEARCH_EDIT    1001
#define IDC_PROGRAMS_LIST  1002

// ============================================================================
// Menu Item Definitions
// ============================================================================
struct MenuEntry {
    std::wstring displayName;
    std::wstring targetPath;    // Shell path or CSIDL-based path
    HICON        hIcon;
    bool         isSeparator;
    bool         isPowerItem;
};

static std::vector<MenuEntry> g_programItems;
static std::vector<MenuEntry> g_userShortcuts;
static std::vector<MenuEntry> g_powerItems;

// ============================================================================
// Registry Configuration
// ============================================================================
static void LoadConfiguration() {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
        L"Software\\EliteSoftware\\Win32Explorer\\Settings",
        0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // Read any start menu specific settings here
        // Future: StartMenuStyle, StartMenuSkin, PinnedItems, etc.
        RegCloseKey(hKey);
    }
}

// ============================================================================
// Shell Folder Enumeration - Read Start Menu Programs
// ============================================================================
static void EnumerateStartMenuFolder(const wchar_t* folderPath, std::vector<MenuEntry>& items) {
    if (!folderPath || !PathFileExistsW(folderPath)) return;

    WIN32_FIND_DATAW fd;
    wchar_t searchPath[MAX_PATH];
    swprintf_s(searchPath, L"%s\\*", folderPath);

    HANDLE hFind = FindFirstFileW(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (fd.cFileName[0] == L'.') continue;

        wchar_t fullPath[MAX_PATH];
        swprintf_s(fullPath, L"%s\\%s", folderPath, fd.cFileName);

        // Skip directories for now (flat list), but could recurse
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        // Only include .lnk shortcuts and .exe files
        const wchar_t* ext = PathFindExtensionW(fd.cFileName);
        if (_wcsicmp(ext, L".lnk") != 0 && _wcsicmp(ext, L".exe") != 0) continue;

        MenuEntry entry;
        entry.isSeparator = false;
        entry.isPowerItem = false;

        // Get display name (strip .lnk extension)
        entry.displayName = fd.cFileName;
        size_t dotPos = entry.displayName.rfind(L'.');
        if (dotPos != std::wstring::npos) {
            entry.displayName = entry.displayName.substr(0, dotPos);
        }

        entry.targetPath = fullPath;

        // Extract icon from file
        SHFILEINFOW sfi = {};
        if (SHGetFileInfoW(fullPath, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
            entry.hIcon = sfi.hIcon;
        } else {
            entry.hIcon = NULL;
        }

        items.push_back(entry);
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);

    // Sort alphabetically
    std::sort(items.begin(), items.end(),
        [](const MenuEntry& a, const MenuEntry& b) {
            return _wcsicmp(a.displayName.c_str(), b.displayName.c_str()) < 0;
        });
}

static void PopulateProgramsList() {
    // Clear existing
    for (auto& item : g_programItems) {
        if (item.hIcon) DestroyIcon(item.hIcon);
    }
    g_programItems.clear();

    // Common Start Menu\Programs
    wchar_t commonPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, commonPath) == S_OK) {
        EnumerateStartMenuFolder(commonPath, g_programItems);
    }

    // User Start Menu\Programs
    wchar_t userPath[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, userPath) == S_OK) {
        EnumerateStartMenuFolder(userPath, g_programItems);
    }
}

static void PopulateUserShortcuts() {
    for (auto& item : g_userShortcuts) {
        if (item.hIcon) DestroyIcon(item.hIcon);
    }
    g_userShortcuts.clear();

    struct ShortcutDef {
        const wchar_t* name;
        int csidl;
    };

    ShortcutDef shortcuts[] = {
        { L"Documents",      CSIDL_PERSONAL },
        { L"Pictures",       CSIDL_MYPICTURES },
        { L"Computer",       -1 },   // Special: ::{20D04FE0-...}
        { L"Control Panel",  -2 },   // Special: ::{26EE0668-...}
    };

    for (const auto& sd : shortcuts) {
        MenuEntry entry;
        entry.displayName = sd.name;
        entry.isSeparator = false;
        entry.isPowerItem = false;

        if (sd.csidl >= 0) {
            wchar_t folderPath[MAX_PATH];
            if (SHGetFolderPathW(NULL, sd.csidl, NULL, 0, folderPath) == S_OK) {
                entry.targetPath = folderPath;
            }
            SHFILEINFOW sfi = {};
            if (!entry.targetPath.empty() &&
                SHGetFileInfoW(entry.targetPath.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON)) {
                entry.hIcon = sfi.hIcon;
            } else {
                entry.hIcon = NULL;
            }
        } else if (sd.csidl == -1) {
            // Computer (This PC)
            entry.targetPath = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}";
            SHFILEINFOW sfi = {};
            LPITEMIDLIST pidl = NULL;
            if (SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidl) == S_OK) {
                if (SHGetFileInfoW((LPCWSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_PIDL)) {
                    entry.hIcon = sfi.hIcon;
                }
                CoTaskMemFree(pidl);
            }
        } else if (sd.csidl == -2) {
            // Control Panel
            entry.targetPath = L"::{26EE0668-A00A-44D7-9371-BEB064C98683}";
            SHFILEINFOW sfi = {};
            LPITEMIDLIST pidl = NULL;
            if (SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidl) == S_OK) {
                if (SHGetFileInfoW((LPCWSTR)pidl, 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_PIDL)) {
                    entry.hIcon = sfi.hIcon;
                }
                CoTaskMemFree(pidl);
            }
        }
        g_userShortcuts.push_back(entry);
    }
}

static void PopulatePowerItems() {
    for (auto& item : g_powerItems) {
        if (item.hIcon) DestroyIcon(item.hIcon);
    }
    g_powerItems.clear();

    struct PowerDef {
        const wchar_t* name;
        const wchar_t* cmd;
    };

    PowerDef items[] = {
        { L"Shut Down",  L"shutdown" },
        { L"Restart",    L"restart" },
        { L"Log Off",    L"logoff" },
        { L"Lock",       L"lock" },
    };

    for (const auto& pd : items) {
        MenuEntry entry;
        entry.displayName = pd.name;
        entry.targetPath  = pd.cmd;
        entry.hIcon       = NULL;
        entry.isSeparator = false;
        entry.isPowerItem = true;
        g_powerItems.push_back(entry);
    }
}

// ============================================================================
// Power Actions
// ============================================================================
static void ExecutePowerAction(const wchar_t* action) {
    if (wcscmp(action, L"shutdown") == 0) {
        // Initiate shutdown
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            LookupPrivilegeValueW(NULL, L"SeShutdownPrivilege", &tkp.Privileges[0].Luid);
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_SHUTDOWN | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
    } else if (wcscmp(action, L"restart") == 0) {
        HANDLE hToken;
        TOKEN_PRIVILEGES tkp;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            LookupPrivilegeValueW(NULL, L"SeShutdownPrivilege", &tkp.Privileges[0].Luid);
            tkp.PrivilegeCount = 1;
            tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
            AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, NULL);
            CloseHandle(hToken);
        }
        ExitWindowsEx(EWX_REBOOT | EWX_FORCE, SHTDN_REASON_MAJOR_OTHER);
    } else if (wcscmp(action, L"logoff") == 0) {
        ExitWindowsEx(EWX_LOGOFF, 0);
    } else if (wcscmp(action, L"lock") == 0) {
        LockWorkStation();
    }
}

// ============================================================================
// Launch Item
// ============================================================================
static void LaunchItem(const MenuEntry& entry) {
    if (entry.isPowerItem) {
        ExecutePowerAction(entry.targetPath.c_str());
        return;
    }

    if (entry.targetPath.empty()) return;

    // Shell namespace items (CLSID paths)
    if (entry.targetPath[0] == L':') {
        ShellExecuteW(NULL, L"open", L"explorer.exe", entry.targetPath.c_str(), NULL, SW_SHOWNORMAL);
    } else {
        ShellExecuteW(NULL, L"open", entry.targetPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
}

// ============================================================================
// Monitor Geometry Helpers
// ============================================================================
struct MonitorInfo {
    HMONITOR hMonitor;
    RECT     rcMonitor;
    RECT     rcWork;
};
static std::vector<MonitorInfo> g_monitors;

static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM) {
    MONITORINFO mi = { sizeof(MONITORINFO) };
    if (GetMonitorInfoW(hMonitor, &mi)) {
        MonitorInfo info;
        info.hMonitor  = hMonitor;
        info.rcMonitor = mi.rcMonitor;
        info.rcWork    = mi.rcWork;
        g_monitors.push_back(info);
    }
    return TRUE;
}

static void RefreshMonitorList() {
    g_monitors.clear();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

// ============================================================================
// Start Menu Popup Window
// ============================================================================

// Custom owner-draw item data stored in list items
struct DrawItemData {
    int sectionIndex;   // 0=programs, 1=user shortcuts, 2=separator, 3=power
    int itemIndex;
};

static HWND  g_hSearchEdit   = NULL;
static HWND  g_hProgramsList = NULL;  // Listbox for programs
static HFONT g_hMenuFont     = NULL;
static HFONT g_hBoldFont     = NULL;
static HFONT g_hSearchFont   = NULL;

static void CreateMenuFonts() {
    if (g_hMenuFont) return;

    NONCLIENTMETRICSW ncm = { sizeof(NONCLIENTMETRICSW) };
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);

    g_hMenuFont = CreateFontIndirectW(&ncm.lfMenuFont);

    ncm.lfMenuFont.lfWeight = FW_BOLD;
    g_hBoldFont = CreateFontIndirectW(&ncm.lfMenuFont);

    ncm.lfMenuFont.lfWeight = FW_NORMAL;
    ncm.lfMenuFont.lfHeight = -14;
    g_hSearchFont = CreateFontIndirectW(&ncm.lfMenuFont);
}

static void DestroyMenuFonts() {
    if (g_hMenuFont)   { DeleteObject(g_hMenuFont); g_hMenuFont = NULL; }
    if (g_hBoldFont)   { DeleteObject(g_hBoldFont); g_hBoldFont = NULL; }
    if (g_hSearchFont) { DeleteObject(g_hSearchFont); g_hSearchFont = NULL; }
}

static void RebuildListbox(const wchar_t* filter) {
    if (!g_hProgramsList) return;
    SendMessageW(g_hProgramsList, LB_RESETCONTENT, 0, 0);

    // Section header: Programs
    int idx;

    // Add program items (filtered)
    for (size_t i = 0; i < g_programItems.size(); i++) {
        if (filter && filter[0] != L'\0') {
            // Simple case-insensitive substring filter
            std::wstring lower = g_programItems[i].displayName;
            std::wstring lowerFilter = filter;
            for (auto& ch : lower) ch = towlower(ch);
            for (auto& ch : lowerFilter) ch = towlower(ch);
            if (lower.find(lowerFilter) == std::wstring::npos) continue;
        }
        idx = (int)SendMessageW(g_hProgramsList, LB_ADDSTRING, 0, (LPARAM)g_programItems[i].displayName.c_str());
        SendMessageW(g_hProgramsList, LB_SETITEMDATA, idx, (LPARAM)(0x00000000 | (int)i)); // section 0, item i
    }

    // Only show user shortcuts and power items if no filter active
    if (!filter || filter[0] == L'\0') {
        // Separator
        idx = (int)SendMessageW(g_hProgramsList, LB_ADDSTRING, 0, (LPARAM)L"---");
        SendMessageW(g_hProgramsList, LB_SETITEMDATA, idx, (LPARAM)0xFFFFFFFE); // separator marker

        // User shortcuts
        for (size_t i = 0; i < g_userShortcuts.size(); i++) {
            idx = (int)SendMessageW(g_hProgramsList, LB_ADDSTRING, 0, (LPARAM)g_userShortcuts[i].displayName.c_str());
            SendMessageW(g_hProgramsList, LB_SETITEMDATA, idx, (LPARAM)(0x10000000 | (int)i)); // section 1
        }

        // Separator
        idx = (int)SendMessageW(g_hProgramsList, LB_ADDSTRING, 0, (LPARAM)L"---");
        SendMessageW(g_hProgramsList, LB_SETITEMDATA, idx, (LPARAM)0xFFFFFFFE);

        // Power items
        for (size_t i = 0; i < g_powerItems.size(); i++) {
            idx = (int)SendMessageW(g_hProgramsList, LB_ADDSTRING, 0, (LPARAM)g_powerItems[i].displayName.c_str());
            SendMessageW(g_hProgramsList, LB_SETITEMDATA, idx, (LPARAM)(0x30000000 | (int)i)); // section 3
        }
    }
}

static MenuEntry* GetMenuEntryFromItemData(LPARAM data) {
    if (data == (LPARAM)0xFFFFFFFE) return NULL; // separator
    int section = (int)((data >> 28) & 0xF);
    int index   = (int)(data & 0x0FFFFFFF);

    if (section == 0 && index >= 0 && index < (int)g_programItems.size())
        return &g_programItems[index];
    if (section == 1 && index >= 0 && index < (int)g_userShortcuts.size())
        return &g_userShortcuts[index];
    if (section == 3 && index >= 0 && index < (int)g_powerItems.size())
        return &g_powerItems[index];

    return NULL;
}

// ============================================================================
// Start Menu Window Procedure
// ============================================================================
static LRESULT CALLBACK StartMenuWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            CreateMenuFonts();

            // Search box
            g_hSearchEdit = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                10, 10, MENU_WIDTH - 20, 24,
                hwnd, (HMENU)(UINT_PTR)IDC_SEARCH_EDIT, g_hInstance, NULL);
            if (g_hSearchEdit && g_hSearchFont) {
                SendMessageW(g_hSearchEdit, WM_SETFONT, (WPARAM)g_hSearchFont, TRUE);
            }
            // Set cue banner (placeholder text)
            SendMessageW(g_hSearchEdit, EM_SETCUEBANNER, TRUE, (LPARAM)L"Search programs and files...");

            // Programs listbox (owner-draw fixed for icons)
            g_hProgramsList = CreateWindowExW(
                WS_EX_CLIENTEDGE,
                L"LISTBOX", L"",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_NOTIFY | LBS_NOINTEGRALHEIGHT,
                10, 44, MENU_WIDTH - 20, MENU_HEIGHT - 60,
                hwnd, (HMENU)(UINT_PTR)IDC_PROGRAMS_LIST, g_hInstance, NULL);
            if (g_hProgramsList && g_hMenuFont) {
                SendMessageW(g_hProgramsList, WM_SETFONT, (WPARAM)g_hMenuFont, TRUE);
                // Set item height for icons (20px icon + 4px padding)
                SendMessageW(g_hProgramsList, LB_SETITEMHEIGHT, 0, MAKELPARAM(24, 0));
            }

            // Populate data
            PopulateProgramsList();
            PopulateUserShortcuts();
            PopulatePowerItems();
            RebuildListbox(NULL);

            return 0;
        }

        case WM_COMMAND: {
            WORD notif = HIWORD(wParam);
            WORD id    = LOWORD(wParam);

            if (id == IDC_SEARCH_EDIT && notif == EN_CHANGE) {
                // Filter the list based on search text
                wchar_t searchText[256] = {};
                GetWindowTextW(g_hSearchEdit, searchText, 256);
                RebuildListbox(searchText);
                return 0;
            }

            if (id == IDC_PROGRAMS_LIST && notif == LBN_DBLCLK) {
                int sel = (int)SendMessageW(g_hProgramsList, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    LPARAM data = SendMessageW(g_hProgramsList, LB_GETITEMDATA, sel, 0);
                    MenuEntry* entry = GetMenuEntryFromItemData(data);
                    if (entry) {
                        HideStartMenu();
                        LaunchItem(*entry);
                    }
                }
                return 0;
            }
            break;
        }

        case WM_MEASUREITEM: {
            MEASUREITEMSTRUCT* mis = (MEASUREITEMSTRUCT*)lParam;
            if (mis->CtlID == IDC_PROGRAMS_LIST) {
                mis->itemHeight = 24;
                return TRUE;
            }
            break;
        }

        case WM_DRAWITEM: {
            DRAWITEMSTRUCT* dis = (DRAWITEMSTRUCT*)lParam;
            if (dis->CtlID != IDC_PROGRAMS_LIST) break;
            if (dis->itemID == (UINT)-1) break;

            HDC hdc = dis->hDC;
            RECT rc = dis->rcItem;
            LPARAM data = dis->itemData;

            // Check if separator
            if (data == (LPARAM)0xFFFFFFFE) {
                FillRect(hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
                int midY = (rc.top + rc.bottom) / 2;
                HPEN hPen = CreatePen(PS_SOLID, 1, CLR_SEPARATOR);
                HPEN hOld = (HPEN)SelectObject(hdc, hPen);
                MoveToEx(hdc, rc.left + 4, midY, NULL);
                LineTo(hdc, rc.right - 4, midY);
                SelectObject(hdc, hOld);
                DeleteObject(hPen);
                return TRUE;
            }

            // Background
            bool selected = (dis->itemState & ODS_SELECTED) != 0;
            COLORREF bgColor = selected ? CLR_HOVER_BG : RGB(255, 255, 255);
            HBRUSH hBrush = CreateSolidBrush(bgColor);
            FillRect(hdc, &rc, hBrush);
            DeleteObject(hBrush);

            // Focus rect
            if (dis->itemState & ODS_FOCUS) {
                DrawFocusRect(hdc, &rc);
            }

            // Get the menu entry
            MenuEntry* entry = GetMenuEntryFromItemData(data);
            if (!entry) break;

            // Draw icon
            int iconX = rc.left + 4;
            int iconY = rc.top + (rc.bottom - rc.top - 16) / 2;
            if (entry->hIcon) {
                DrawIconEx(hdc, iconX, iconY, entry->hIcon, 16, 16, 0, NULL, DI_NORMAL);
            }

            // Draw text
            RECT textRc = rc;
            textRc.left = iconX + 20;
            textRc.right -= 4;
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, CLR_TEXT_NORMAL);

            HFONT hOldFont = NULL;
            if (entry->isPowerItem && g_hBoldFont) {
                hOldFont = (HFONT)SelectObject(hdc, g_hBoldFont);
            } else if (g_hMenuFont) {
                hOldFont = (HFONT)SelectObject(hdc, g_hMenuFont);
            }

            DrawTextW(hdc, entry->displayName.c_str(), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

            if (hOldFont) SelectObject(hdc, hOldFont);

            return TRUE;
        }

        case WM_ACTIVATE: {
            if (LOWORD(wParam) == WA_INACTIVE) {
                // Hide menu when user clicks outside
                HideStartMenu();
            }
            return 0;
        }

        case WM_KEYDOWN: {
            if (wParam == VK_ESCAPE) {
                HideStartMenu();
                return 0;
            }
            break;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            // Fill background
            RECT rc;
            GetClientRect(hwnd, &rc);
            HBRUSH hBg = CreateSolidBrush(CLR_MENU_BG);
            FillRect(hdc, &rc, hBg);
            DeleteObject(hBg);

            // Draw sidebar (left strip, Vista/Win7 style)
            RECT sidebar = { 0, 0, 4, rc.bottom };
            HBRUSH hSide = CreateSolidBrush(CLR_SIDEBAR_BG);
            FillRect(hdc, &sidebar, hSide);
            DeleteObject(hSide);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_DESTROY: {
            g_hSearchEdit   = NULL;
            g_hProgramsList = NULL;
            g_hMenuWnd      = NULL;
            g_bMenuVisible  = false;
            return 0;
        }
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// ============================================================================
// Show / Hide / Toggle
// ============================================================================
static void ShowStartMenu(int monitorIndex, int taskbarY) {
    if (g_bMenuVisible && g_hMenuWnd) {
        HideStartMenu();
        return;
    }

    RefreshMonitorList();

    // Determine position: above the taskbar on the target monitor
    RECT workArea = { 0, 0, 0, 0 };
    if (monitorIndex >= 0 && monitorIndex < (int)g_monitors.size()) {
        workArea = g_monitors[monitorIndex].rcWork;
    } else if (!g_monitors.empty()) {
        workArea = g_monitors[0].rcWork;
    } else {
        SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    }

    int x = workArea.left;
    int y = workArea.bottom - MENU_HEIGHT;

    // If taskbarY was passed, position relative to it
    if (taskbarY > 0 && taskbarY < workArea.bottom) {
        y = taskbarY - MENU_HEIGHT;
    }

    // Clamp to screen
    if (y < workArea.top) y = workArea.top;

    // Register window class if not done
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSEXW wc = { sizeof(WNDCLASSEXW) };
        wc.lpfnWndProc   = StartMenuWndProc;
        wc.hInstance      = g_hInstance;
        wc.lpszClassName  = L"EliteStartMenuPopup";
        wc.hCursor        = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
        wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
        wc.style          = CS_DROPSHADOW;
        RegisterClassExW(&wc);
        classRegistered = true;
    }

    g_hMenuWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"EliteStartMenuPopup",
        L"Start",
        WS_POPUP | WS_BORDER,
        x, y, MENU_WIDTH, MENU_HEIGHT,
        NULL, NULL, g_hInstance, NULL);

    if (g_hMenuWnd) {
        // Enable visual styles on the window
        SetWindowTheme(g_hMenuWnd, L"", L"");

        ShowWindow(g_hMenuWnd, SW_SHOWNOACTIVATE);
        SetForegroundWindow(g_hMenuWnd);
        // Focus the search box
        if (g_hSearchEdit) {
            SetFocus(g_hSearchEdit);
        }
        g_bMenuVisible = true;
    }
}

static void HideStartMenu() {
    if (g_hMenuWnd) {
        DestroyWindow(g_hMenuWnd);
        g_hMenuWnd = NULL;
    }
    g_bMenuVisible = false;
}

static void ToggleStartMenu() {
    if (g_bMenuVisible) {
        HideStartMenu();
    } else {
        ShowStartMenu(g_monitorIndex, g_taskbarY);
    }
}

static void LaunchSettings() {
    // Find EliteTaskbar.exe next to us and launch with -settings
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"EliteTaskbar.exe");

    if (PathFileExistsW(path)) {
        ShellExecuteW(NULL, L"open", path, L"-settings", NULL, SW_SHOWNORMAL);
    }
}

// ============================================================================
// IPC Window Procedure (hidden message-only window)
// ============================================================================
static LRESULT CALLBACK IPCWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == g_WM_TOGGLE && g_WM_TOGGLE != 0) {
        // wParam = monitor index, lParam = taskbar Y
        g_monitorIndex = (int)wParam;
        g_taskbarY     = (int)lParam;
        ToggleStartMenu();
        return 0;
    }

    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// ============================================================================
// Command Line Parsing
// ============================================================================
enum class StartupAction {
    None,
    Toggle,
    Show,
    Hide,
    Exit,
    Settings
};

static StartupAction ParseCommandLine(int& outMonitor, int& outTaskbarY) {
    StartupAction action = StartupAction::None;
    outMonitor  = 0;
    outTaskbarY = 0;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv) return action;

    for (int i = 1; i < argc; i++) {
        if (_wcsicmp(argv[i], L"-toggle") == 0) {
            action = StartupAction::Toggle;
        } else if (_wcsicmp(argv[i], L"-show") == 0) {
            action = StartupAction::Show;
        } else if (_wcsicmp(argv[i], L"-hide") == 0) {
            action = StartupAction::Hide;
        } else if (_wcsicmp(argv[i], L"-exit") == 0) {
            action = StartupAction::Exit;
        } else if (_wcsicmp(argv[i], L"-settings") == 0) {
            action = StartupAction::Settings;
        } else if (_wcsicmp(argv[i], L"-monitor") == 0 || _wcsicmp(argv[i], L"-MonitorIndex") == 0) {
            if (i + 1 < argc) {
                outMonitor = _wtoi(argv[i + 1]);
                i++;
            }
        } else if (_wcsicmp(argv[i], L"-TaskbarY") == 0) {
            if (i + 1 < argc) {
                outTaskbarY = _wtoi(argv[i + 1]);
                i++;
            }
        }
    }

    LocalFree(argv);
    return action;
}

// ============================================================================
// Send IPC message to existing instance
// ============================================================================
static bool SendIPCMessage(UINT action, int monitorIndex, int taskbarY) {
    HWND hExisting = FindWindowW(L"EliteStartMenuIPC", NULL);
    if (!hExisting) return false;

    if (action == 0) {
        // Toggle
        SendMessageW(hExisting, g_WM_TOGGLE, (WPARAM)monitorIndex, (LPARAM)taskbarY);
    } else if (action == 1) {
        // Exit
        SendMessageW(hExisting, WM_CLOSE, 0, 0);
    }
    return true;
}

// ============================================================================
// Cleanup
// ============================================================================
static void CleanupAll() {
    HideStartMenu();

    // Free program item icons
    for (auto& item : g_programItems)   { if (item.hIcon) DestroyIcon(item.hIcon); }
    for (auto& item : g_userShortcuts)  { if (item.hIcon) DestroyIcon(item.hIcon); }
    for (auto& item : g_powerItems)     { if (item.hIcon) DestroyIcon(item.hIcon); }

    g_programItems.clear();
    g_userShortcuts.clear();
    g_powerItems.clear();

    DestroyMenuFonts();

    if (g_hIPCWnd) {
        DestroyWindow(g_hIPCWnd);
        g_hIPCWnd = NULL;
    }

    if (g_hMutex) {
        ReleaseMutex(g_hMutex);
        CloseHandle(g_hMutex);
        g_hMutex = NULL;
    }
}

// ============================================================================
// Entry Point
// ============================================================================
int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    g_hInstance = hInstance;

    // Register the custom window message for IPC (before mutex check so we can forward)
    g_WM_TOGGLE = RegisterWindowMessageW(L"EliteStartMenu_Toggle");

    // Parse command line
    int cmdMonitor = 0, cmdTaskbarY = 0;
    StartupAction action = ParseCommandLine(cmdMonitor, cmdTaskbarY);
    g_monitorIndex = cmdMonitor;
    g_taskbarY     = cmdTaskbarY;

    // Handle -exit for any instance
    if (action == StartupAction::Exit) {
        HWND hExisting = FindWindowW(L"EliteStartMenuIPC", NULL);
        if (hExisting) {
            PostMessageW(hExisting, WM_CLOSE, 0, 0);
        }
        return 0;
    }

    // Handle -settings (just launch settings and exit)
    if (action == StartupAction::Settings) {
        LaunchSettings();
        return 0;
    }

    // Single-instance mutex
    g_hMutex = CreateMutexW(NULL, TRUE, L"EliteStartMenu.Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        // Another instance is running - send IPC command and exit
        if (action == StartupAction::Toggle || action == StartupAction::None) {
            HWND hExisting = FindWindowW(L"EliteStartMenuIPC", NULL);
            if (hExisting) {
                PostMessageW(hExisting, g_WM_TOGGLE, (WPARAM)cmdMonitor, (LPARAM)cmdTaskbarY);
            }
        } else if (action == StartupAction::Show) {
            HWND hExisting = FindWindowW(L"EliteStartMenuIPC", NULL);
            if (hExisting) {
                PostMessageW(hExisting, g_WM_TOGGLE, (WPARAM)cmdMonitor, (LPARAM)cmdTaskbarY);
            }
        } else if (action == StartupAction::Hide) {
            HWND hExisting = FindWindowW(L"EliteStartMenuIPC", NULL);
            if (hExisting) {
                PostMessageW(hExisting, g_WM_TOGGLE, (WPARAM)cmdMonitor, (LPARAM)cmdTaskbarY);
            }
        }
        CloseHandle(g_hMutex);
        return 0;
    }

    // Initialize COM
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    // Initialize Common Controls (visual styles)
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_COOL_CLASSES;
    InitCommonControlsEx(&icex);

    // UIPI Bypass: Allow lower-elevation processes (EliteTaskbar) to send messages to us
    for (UINT msg = 0; msg <= 0xFFFF; ++msg) {
        ChangeWindowMessageFilter(msg, MSGFLT_ADD);
    }

    // Load configuration from registry
    LoadConfiguration();

    // Register IPC window class
    WNDCLASSEXW ipcWc = { sizeof(WNDCLASSEXW) };
    ipcWc.lpfnWndProc   = IPCWndProc;
    ipcWc.hInstance      = hInstance;
    ipcWc.lpszClassName  = L"EliteStartMenuIPC";
    RegisterClassExW(&ipcWc);

    // Create hidden message-only window for IPC
    g_hIPCWnd = CreateWindowExW(
        0,
        L"EliteStartMenuIPC",
        L"EliteStartMenuIPC",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,  // Message-only window
        NULL, hInstance, NULL);

    if (!g_hIPCWnd) {
        CoUninitialize();
        CleanupAll();
        return -1;
    }

    // If launched with -toggle or -show, immediately show the menu
    if (action == StartupAction::Toggle || action == StartupAction::Show || action == StartupAction::None) {
        // Post ourselves a toggle message so it happens after the message loop starts
        PostMessageW(g_hIPCWnd, g_WM_TOGGLE, (WPARAM)cmdMonitor, (LPARAM)cmdTaskbarY);
    }

    // Message loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        // Allow ESC to close from any child control
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            HideStartMenu();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup
    CleanupAll();
    CoUninitialize();

    return (int)msg.wParam;
}
