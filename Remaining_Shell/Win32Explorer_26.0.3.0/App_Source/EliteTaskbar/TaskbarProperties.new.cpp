#include "TaskbarWindow.h"
#include "resource.h"
#include "Config.h"
#include "Logger.h"
#include "StartButton.h"
#include <commctrl.h>
#include <vector>

void UpdateOrbPreview(HWND hwndDlg, DWORD orbId) {
    HBITMAP hBitmap = (HBITMAP)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(orbId), IMAGE_BITMAP, 54, 54, LR_DEFAULTCOLOR);
    if (hBitmap) {
        HBITMAP hOld = (HBITMAP)SendDlgItemMessageW(hwndDlg, IDC_ORB_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
        if (hOld) DeleteObject(hOld);
    }
}

void SaveToNativeRegistry(LPCWSTR valueName, DWORD value) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void NotifySettingsChange() {
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
}

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0;
            DWORD cbData = sizeof(DWORD);
            
            if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarButtonWidthMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_AUTO, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_FIXED, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_ICONS, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_WIDTH_AUTO, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarHoverPreview", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                SendDlgItemMessageW(hwndDlg, IDC_HOVER_PREVIEW, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            RegCloseKey(hKey);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD mode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 2;
                RegSetValueExW(hKey, L"TaskbarMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                
                DWORD widthMode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_WIDTH_FIXED, BM_GETCHECK, 0, 0) == BST_CHECKED) widthMode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_WIDTH_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) widthMode = 2;
                RegSetValueExW(hKey, L"TaskbarButtonWidthMode", 0, REG_DWORD, (const BYTE*)&widthMode, sizeof(DWORD));
                
                DWORD hoverPreview = (SendDlgItemMessageW(hwndDlg, IDC_HOVER_PREVIEW, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TaskbarHoverPreview", 0, REG_DWORD, (const BYTE*)&hoverPreview, sizeof(DWORD));
                
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK NativeSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0, cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"NativeRegistryMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_NATIVE_REGISTRY_MODE, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
                
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"UseNativeTaskBand", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_USE_NATIVE_TASKBAND, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
                
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TrayMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_SETCHECK, BST_CHECKED, 0);
                else SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
            }
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0, cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_SETCHECK, (dwValue == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
                
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            BYTE blob[64];
            DWORD cbData = sizeof(blob);
            if (RegQueryValueExW(hKey, L"Settings", NULL, NULL, blob, &cbData) == ERROR_SUCCESS && cbData >= 9) {
                bool autoHide = (blob[8] & 0x01) != 0;
                SendDlgItemMessageW(hwndDlg, IDC_AUTOHIDE_TASKBAR, BM_SETCHECK, autoHide ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            RegCloseKey(hKey);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD nativeSync = (SendDlgItemMessageW(hwndDlg, IDC_NATIVE_REGISTRY_MODE, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"NativeRegistryMode", 0, REG_DWORD, (const BYTE*)&nativeSync, sizeof(DWORD));
                
                DWORD useNativeBand = (SendDlgItemMessageW(hwndDlg, IDC_USE_NATIVE_TASKBAND, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"UseNativeTaskBand", 0, REG_DWORD, (const BYTE*)&useNativeBand, sizeof(DWORD));
                
                DWORD trayMode = (SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TrayMode", 0, REG_DWORD, (const BYTE*)&trayMode, sizeof(DWORD));
                RegCloseKey(hKey);
            }
            
            DWORD locked = (SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 0 : 1;
            SaveToNativeRegistry(L"TaskbarSizeMove", locked);
            DWORD smallIcons = (SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            SaveToNativeRegistry(L"TaskbarSmallIcons", smallIcons);
            
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                BYTE blob[64];
                DWORD cbData = sizeof(blob);
                if (RegQueryValueExW(hKey, L"Settings", NULL, NULL, blob, &cbData) == ERROR_SUCCESS && cbData >= 9) {
                    if (SendDlgItemMessageW(hwndDlg, IDC_AUTOHIDE_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) blob[8] |= 0x01;
                    else blob[8] &= ~0x01;
                    RegSetValueExW(hKey, L"Settings", 0, REG_BINARY, blob, cbData);
                }
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

LRESULT CALLBACK DynScrollAreaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_VSCROLL: {
            SCROLLINFO si = { sizeof(si), SIF_ALL };
            GetScrollInfo(hwnd, SB_VERT, &si);
            int oldPos = si.nPos;
            switch (LOWORD(wParam)) {
                case SB_TOP: si.nPos = si.nMin; break;
                case SB_BOTTOM: si.nPos = si.nMax; break;
                case SB_LINEUP: si.nPos -= 20; break;
                case SB_LINEDOWN: si.nPos += 20; break;
                case SB_PAGEUP: si.nPos -= si.nPage; break;
                case SB_PAGEDOWN: si.nPos += si.nPage; break;
                case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
            }
            if (si.nPos < 0) si.nPos = 0;
            if (si.nPos > si.nMax - (int)si.nPage + 1) si.nPos = si.nMax - (int)si.nPage + 1;
            if (si.nPos < 0) si.nPos = 0;
            if (si.nPos != oldPos) {
                ScrollWindowEx(hwnd, 0, oldPos - si.nPos, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
                SetScrollPos(hwnd, SB_VERT, si.nPos, TRUE);
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            SendMessageW(hwnd, WM_VSCROLL, zDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            SendMessageW(hwnd, WM_VSCROLL, zDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            SendMessageW(hwnd, WM_VSCROLL, zDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            return 0;
        }
        case WM_COMMAND:
            SendMessageW(GetParent(hwnd), WM_COMMAND, wParam, lParam);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void InitDynScrollClass() {
    static bool init = false;
    if (init) return;
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = DynScrollAreaProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"EliteDynScrollArea";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    RegisterClassW(&wc);
    init = true;
}

// Global dynamically populated IDs base
#define ID_BASE_MM_TRAY 10000
#define ID_BASE_MM_CLOCK 11000
#define ID_BASE_MM_TBTN 12000
#define ID_BASE_SM_ORB 13000
#define ID_BASE_SM_TRIG 14000
#define ID_BASE_SM_PREV 15000

HWND CreateDynScrollArea(HWND hwndDlg, int idc_placeholder) {
    InitDynScrollClass();
    HWND hPlaceholder = GetDlgItem(hwndDlg, idc_placeholder);
    RECT rc; GetWindowRect(hPlaceholder, &rc);
    POINT pt = { rc.left, rc.top };
    ScreenToClient(hwndDlg, &pt);
    HWND hScroll = CreateWindowExW(0, L"EliteDynScrollArea", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, pt.x, pt.y, rc.right - rc.left, rc.bottom - rc.top, hwndDlg, NULL, GetModuleHandle(NULL), NULL);
    DestroyWindow(hPlaceholder);
    return hScroll;
}

struct MonitorInfo {
    int index;
    HMONITOR hMonitor;
    RECT rect;
};
std::vector<MonitorInfo> g_Monitors;
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorInfo info;
    info.index = (int)g_Monitors.size();
    info.hMonitor = hMonitor;
    info.rect = *lprcMonitor;
    g_Monitors.push_back(info);
    return TRUE;
}

INT_PTR CALLBACK MultiMonSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hScroll = NULL;
    switch (uMsg) {
    case WM_INITDIALOG: {
        hScroll = CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
        g_Monitors.clear();
        EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
        
        int y = 5;
        HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
        
        HKEY hKey;
        RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);
        
        for (const auto& mon : g_Monitors) {
            WCHAR title[64];
            wsprintfW(title, L"Monitor %d (%dx%d)", mon.index, mon.rect.right - mon.rect.left, mon.rect.bottom - mon.rect.top);
            HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, 60, hScroll, NULL, GetModuleHandle(NULL), NULL);
            SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
            
            HWND hChk1 = CreateWindowExW(0, L"Button", L"System Tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 15, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TRAY + mon.index), GetModuleHandle(NULL), NULL);
            HWND hChk2 = CreateWindowExW(0, L"Button", L"Clock", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 120, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_CLOCK + mon.index), GetModuleHandle(NULL), NULL);
            HWND hChk3 = CreateWindowExW(0, L"Button", L"Task Buttons", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 15, y + 40, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TBTN + mon.index), GetModuleHandle(NULL), NULL);
            
            SendMessageW(hChk1, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk2, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk3, WM_SETFONT, (WPARAM)hFont, 0);
            
            if (hKey) {
                DWORD dwValue = 0, cbData = sizeof(DWORD);
                WCHAR val[64];
                wsprintfW(val, L"EnableTray_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                else if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                
                wsprintfW(val, L"EnableClock_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                else SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                
                wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
                else SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            y += 70;
        }
        if (hKey) RegCloseKey(hKey);
        
        SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE, 0, y, 200 };
        SetScrollInfo(hScroll, SB_VERT, &si, TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED) SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                for (const auto& mon : g_Monitors) {
                    DWORD v1 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_TRAY + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v2 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_CLOCK + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v3 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_TBTN + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    
                    WCHAR val[64];
                    wsprintfW(val, L"EnableTray_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v1, sizeof(DWORD));
                    wsprintfW(val, L"EnableClock_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v2, sizeof(DWORD));
                    wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v3, sizeof(DWORD));
                }
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

void PopulateOrbComboBox(HWND hCombo) {
    struct OrbItem { LPCWSTR name; DWORD id; };
    OrbItem items[] = {
        { L"Classic Orb", IDB_START_ORB },
        { L"1Orb", IDB_START_ORB_1ORB },
        { L"Aqua Bottom", IDB_START_ORB_AQUABOTTOM },
        { L"Dunes", IDB_START_ORB_DUNES },
        { L"Indigo", IDB_START_ORB_INDIGO },
        { L"Sapphire", IDB_START_ORB_SAPPHIRE },
        { L"Uranus", IDB_START_ORB_URANUS },
        { L"Vienna Bottom", IDB_START_ORB_VIENNABOTTOM }
    };
    for (int i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        int idx = SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)items[i].name);
        SendMessageW(hCombo, CB_SETITEMDATA, idx, items[i].id);
    }
}

void PopulateTriggerComboBox(HWND hCombo) {
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Native Injection");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Elite Custom Menu");
}

INT_PTR CALLBACK StartMenuSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hScroll = NULL;
    switch (uMsg) {
    case WM_INITDIALOG: {
        hScroll = CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
        if (g_Monitors.empty()) EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
        
        int y = 5;
        HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
        
        HKEY hKey;
        RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);
        
        for (const auto& mon : g_Monitors) {
            WCHAR title[64];
            wsprintfW(title, L"Monitor %d Start Menu", mon.index);
            HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 330, 90, hScroll, NULL, GetModuleHandle(NULL), NULL);
            SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
            
            HWND hLbl1 = CreateWindowExW(0, L"Static", L"Start Button Orb:", WS_CHILD | WS_VISIBLE, 15, y + 20, 100, 15, hScroll, NULL, GetModuleHandle(NULL), NULL);
            HWND hCombo = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 120, y + 18, 140, 100, hScroll, (HMENU)(ID_BASE_SM_ORB + mon.index), GetModuleHandle(NULL), NULL);
            PopulateOrbComboBox(hCombo);
            
            HWND hLbl2 = CreateWindowExW(0, L"Static", L"Preview:", WS_CHILD | WS_VISIBLE, 15, y + 40, 50, 15, hScroll, NULL, GetModuleHandle(NULL), NULL);
            HWND hPreview = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | WS_BORDER, 120, y + 40, 54, 54, hScroll, (HMENU)(ID_BASE_SM_PREV + mon.index), GetModuleHandle(NULL), NULL);
            
            HWND hLbl3 = CreateWindowExW(0, L"Static", L"Trigger Behavior:", WS_CHILD | WS_VISIBLE, 180, y + 40, 100, 15, hScroll, NULL, GetModuleHandle(NULL), NULL);
            HWND hTrigger = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL, 180, y + 55, 140, 100, hScroll, (HMENU)(ID_BASE_SM_TRIG + mon.index), GetModuleHandle(NULL), NULL);
            PopulateTriggerComboBox(hTrigger);
            
            SendMessageW(hLbl1, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hCombo, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hLbl2, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hLbl3, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hTrigger, WM_SETFONT, (WPARAM)hFont, 0);
            
            DWORD orbId = IDB_START_ORB;
            DWORD triggerMode = 0;
            if (hKey) {
                DWORD dwValue = 0, cbData = sizeof(DWORD);
                WCHAR val[64];
                wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) orbId = dwValue;
                else if (RegQueryValueExW(hKey, L"StartOrbID", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) orbId = dwValue;
                
                wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) triggerMode = dwValue;
                else if (RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) triggerMode = dwValue;
            }
            
            int count = SendMessageW(hCombo, CB_GETCOUNT, 0, 0);
            for (int i = 0; i < count; i++) {
                if (SendMessageW(hCombo, CB_GETITEMDATA, i, 0) == orbId) { SendMessageW(hCombo, CB_SETCURSEL, i, 0); break; }
            }
            SendMessageW(hTrigger, CB_SETCURSEL, triggerMode, 0);
            
            HBITMAP hBitmap = (HBITMAP)LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(orbId), IMAGE_BITMAP, 54, 54, LR_DEFAULTCOLOR);
            SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
            
            y += 100;
        }
        if (hKey) RegCloseKey(hKey);
        
        SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE, 0, y, 200 };
        SetScrollInfo(hScroll, SB_VERT, &si, TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            int id = LOWORD(wParam);
            if (id >= ID_BASE_SM_ORB && id < ID_BASE_SM_ORB + 32) {
                int monIndex = id - ID_BASE_SM_ORB;
                HWND hCombo = GetDlgItem(hScroll, id);
                int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    DWORD orbId = SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
                    HWND hPreview = GetDlgItem(hScroll, ID_BASE_SM_PREV + monIndex);
                    HBITMAP hBitmap = (HBITMAP)LoadImageW(GetModuleHandle(NULL), MAKEINTRESOURCEW(orbId), IMAGE_BITMAP, 54, 54, LR_DEFAULTCOLOR);
                    HBITMAP hOld = (HBITMAP)SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                    if (hOld) DeleteObject(hOld);
                }
            }
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                for (const auto& mon : g_Monitors) {
                    HWND hCombo = GetDlgItem(hScroll, ID_BASE_SM_ORB + mon.index);
                    int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                    if (sel != CB_ERR) {
                        DWORD orbId = SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
                        WCHAR val[64]; wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                        if (mon.index == 0) RegSetValueExW(hKey, L"StartOrbID", 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                    }
                    
                    HWND hTrigger = GetDlgItem(hScroll, ID_BASE_SM_TRIG + mon.index);
                    DWORD mode = SendMessageW(hTrigger, CB_GETCURSEL, 0, 0);
                    if (mode != CB_ERR) {
                        WCHAR val[64]; wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                        if (mon.index == 0) RegSetValueExW(hKey, L"StartMenuMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                    }
                }
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK ToolbarsSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Address");
        SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Links");
        SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Desktop");
        return TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == LBN_SELCHANGE) SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) { SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR); return TRUE; }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK GenericPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

void ShowTaskbarProperties(HWND hwndOwner) {
    std::vector<HPROPSHEETPAGE> pages;
    PROPSHEETPAGEW psp[6] = {0};
    HPROPSHEETPAGE hPage;
    
    psp[0].dwSize = sizeof(PROPSHEETPAGEW);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = GetModuleHandle(NULL);
    psp[0].pszTemplate = MAKEINTRESOURCEW(IDD_TASKBAR_PROPS);
    psp[0].pfnDlgProc = TaskbarSettingsDlgProc;
    psp[0].pszTitle = L"Taskbar";
    hPage = CreatePropertySheetPageW(&psp[0]);
    if (hPage) pages.push_back(hPage);

    psp[1].dwSize = sizeof(PROPSHEETPAGEW);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = GetModuleHandle(NULL);
    psp[1].pszTemplate = MAKEINTRESOURCEW(IDD_STARTMENU_PROPS);
    psp[1].pfnDlgProc = StartMenuSettingsDlgProc;
    psp[1].pszTitle = L"Start Menu";
    hPage = CreatePropertySheetPageW(&psp[1]);
    if (hPage) pages.push_back(hPage);

    psp[2].dwSize = sizeof(PROPSHEETPAGEW);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = GetModuleHandle(NULL);
    psp[2].pszTemplate = MAKEINTRESOURCEW(IDD_MULTIMON_PROPS);
    psp[2].pfnDlgProc = MultiMonSettingsDlgProc;
    psp[2].pszTitle = L"Multi-Monitor Components";
    hPage = CreatePropertySheetPageW(&psp[2]);
    if (hPage) pages.push_back(hPage);

    psp[3].dwSize = sizeof(PROPSHEETPAGEW);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = GetModuleHandle(NULL);
    psp[3].pszTemplate = MAKEINTRESOURCEW(IDD_NATIVE_PROPS);
    psp[3].pfnDlgProc = NativeSettingsDlgProc;
    psp[3].pszTitle = L"Native Settings";
    hPage = CreatePropertySheetPageW(&psp[3]);
    if (hPage) pages.push_back(hPage);

    psp[4].dwSize = sizeof(PROPSHEETPAGEW);
    psp[4].dwFlags = PSP_USETITLE;
    psp[4].hInstance = GetModuleHandle(NULL);
    psp[4].pszTemplate = MAKEINTRESOURCEW(IDD_TOOLBARS_PROPS);
    psp[4].pfnDlgProc = ToolbarsSettingsDlgProc;
    psp[4].pszTitle = L"Toolbars";
    hPage = CreatePropertySheetPageW(&psp[4]);
    if (hPage) pages.push_back(hPage);

    bool showDebugTabs = false;
    if (wcsstr(GetCommandLineW(), L"/devmode")) showDebugTabs = true;
    
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue = 0, cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"EnableDebugTabs", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue == 1) showDebugTabs = true;
        RegCloseKey(hKey);
    }

    if (showDebugTabs) {
        psp[5].dwSize = sizeof(PROPSHEETPAGEW);
        psp[5].dwFlags = PSP_USETITLE;
        psp[5].hInstance = GetModuleHandle(NULL);
        psp[5].pszTemplate = MAKEINTRESOURCEW(IDD_SECRET_EVERYTHING);
        psp[5].pfnDlgProc = GenericPageDlgProc;
        psp[5].pszTitle = L"Everything Indexer";
        hPage = CreatePropertySheetPageW(&psp[5]);
        if (hPage) pages.push_back(hPage);
    }

    if (pages.empty()) return;

    PROPSHEETHEADERW psh = { sizeof(PROPSHEETHEADERW) };
    psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID;
    psh.hwndParent = hwndOwner;
    psh.hInstance = GetModuleHandle(NULL);
    psh.pszIcon = MAKEINTRESOURCEW(IDI_PREFERENCES);
    psh.pszCaption = L"Taskbar and Start Menu Properties";
    psh.nPages = (UINT)pages.size();
    psh.nStartPage = 0;
    psh.phpage = pages.data();

    PropertySheetW(&psh);
}

INT_PTR CALLBACK SecretDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: return TRUE;
        case WM_COMMAND: if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) { EndDialog(hwndDlg, LOWORD(wParam)); return TRUE; } break;
    }
    return FALSE;
}
