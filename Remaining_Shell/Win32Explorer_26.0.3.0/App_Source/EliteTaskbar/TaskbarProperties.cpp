#include "stdafx.h"
#pragma warning(disable: 4100 4244 4267 4189 4312)
#include "TaskbarWindow.h"
#include "resource.h"
extern HINSTANCE g_hInstance;
#include "Config.h"
#include "Logger.h"
#include "StartButton.h"
#include <commctrl.h>
#include <vector>
#include <string>
#include <gdiplus.h>
#include <uxtheme.h>
#include <shlwapi.h>
#include <shlobj.h>

static void BrowseForFolder(HWND hwndOwner, HWND hwndEdit) {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwndOwner;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    bi.lpszTitle = L"Select Custom Icon Theme Folder:";
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        WCHAR szPath[MAX_PATH];
        if (SHGetPathFromIDListW((PCIDLIST_ABSOLUTE)pidl, szPath)) {
            SetWindowTextW(hwndEdit, szPath);
            SendMessageW(GetParent(hwndEdit), PSM_CHANGED, (WPARAM)GetParent(hwndEdit), 0);
        }
        CoTaskMemFree(pidl);
    }
}

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace Gdiplus;

HBITMAP LoadPngResourceAsHBITMAP(DWORD resId) {
    HBITMAP hBitmap = NULL;
    HRSRC hResource = FindResourceW(g_hInstance, MAKEINTRESOURCEW(resId), (LPCWSTR)RT_RCDATA);
    if (!hResource) return NULL;
    DWORD imageSize = SizeofResource(g_hInstance, hResource);
    HGLOBAL hGlobal = LoadResource(g_hInstance, hResource);
    if (!hGlobal) return NULL;
    const void* pResourceData = LockResource(hGlobal);
    if (pResourceData) {
        HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
        if (hBuffer) {
            void* pBuffer = GlobalLock(hBuffer);
            memcpy(pBuffer, pResourceData, imageSize);
            GlobalUnlock(hBuffer);
            IStream* pStream = NULL;
            if (CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) == S_OK) {
                Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
                if (pBitmap && pBitmap->GetLastStatus() == Gdiplus::Ok) {
                    Gdiplus::Color bg(0, 0, 0, 0);
                    UINT width = pBitmap->GetWidth();
                    UINT height = pBitmap->GetHeight();
                    UINT frameHeight = height / 3;
                    Gdiplus::Bitmap* pResized = new Gdiplus::Bitmap(54, 54, PixelFormat32bppARGB);
                    if (pResized) {
                        Gdiplus::Graphics g(pResized);
                        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                        Gdiplus::Rect destRect(0, 0, 54, 54);
                        g.DrawImage(pBitmap, destRect, 0, 0, width, frameHeight, Gdiplus::UnitPixel);
                        pResized->GetHBITMAP(bg, &hBitmap);
                        delete pResized;
                    } else {
                        pBitmap->GetHBITMAP(bg, &hBitmap);
                    }
                }
                if (pBitmap) delete pBitmap;
                pStream->Release();
            }
        }
    }
    return hBitmap;
}

void UpdateOrbPreview(HWND hwndDlg, DWORD orbId) {
    HBITMAP hBitmap = (HBITMAP)LoadImageW(g_hInstance, MAKEINTRESOURCEW(orbId), IMAGE_BITMAP, 54, 54, LR_DEFAULTCOLOR);
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
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        HKEY hKey;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0;
            DWORD cbData = sizeof(DWORD);
            
            if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            WCHAR szThemePath[MAX_PATH] = {0};
            DWORD cbThemePath = sizeof(szThemePath);
            if (RegQueryValueExW(hKey, L"CustomThemePath", NULL, NULL, (LPBYTE)szThemePath, &cbThemePath) == ERROR_SUCCESS) {
                SetDlgItemTextW(hwndDlg, IDC_THEME_FOLDER_PATH, szThemePath);
            }
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DARK_MODE), FALSE);
            RegCloseKey(hKey);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_THEME_FOLDER_BROWSE) {
            BrowseForFolder(hwndDlg, GetDlgItem(hwndDlg, IDC_THEME_FOLDER_PATH));
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_THEME_FOLDER_PATH && HIWORD(wParam) == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            return TRUE;
        }
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            DWORD portable = 0;
            HKEY hTemp;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hTemp) == ERROR_SUCCESS) {
                DWORD val = 0;
                DWORD size = sizeof(DWORD);
                if (RegQueryValueExW(hTemp, L"EnablePortableMirror", NULL, NULL, (LPBYTE)&val, &size) == ERROR_SUCCESS) {
                    portable = val;
                }
                RegCloseKey(hTemp);
            }

            HKEY hKey;
            HKEY hKeyRoot = (portable == 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
            if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD mode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 2;
                RegSetValueExW(hKey, L"TaskbarMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                
                WCHAR szThemePath[MAX_PATH] = {0};
                GetDlgItemTextW(hwndDlg, IDC_THEME_FOLDER_PATH, szThemePath, MAX_PATH);
                RegSetValueExW(hKey, L"CustomThemePath", 0, REG_SZ, (const BYTE*)szThemePath, (DWORD)(wcslen(szThemePath) + 1) * sizeof(WCHAR));
                
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
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
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
        case WM_ERASEBKGND:
        case WM_PRINTCLIENT:
            DrawThemeParentBackground(hwnd, (HDC)wParam, NULL);
            return 1;
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
    wc.hInstance = g_hInstance;
    wc.lpszClassName = L"EliteDynScrollArea";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    RegisterClassW(&wc);
    init = true;
}

LRESULT CALLBACK NoMouseWheelSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == WM_MOUSEWHEEL) {
        HWND hParent = GetParent(hwnd);
        if (hParent) SendMessageW(hParent, uMsg, wParam, lParam);
        return 0;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// Global dynamically populated IDs base
#define ID_BASE_MM_TRAY 10000
#define ID_BASE_MM_CLOCK 11000
#define ID_BASE_MM_TBTN 12000
#define ID_BASE_SM_ORB 13000
#define ID_BASE_SM_TRIG 14000
#define ID_BASE_SM_MODE 15000
#define ID_BASE_SM_PREV 16000

HWND CreateDynScrollArea(HWND hwndDlg, int idc_placeholder) {
    InitDynScrollClass();
    HWND hPlaceholder = GetDlgItem(hwndDlg, idc_placeholder);
    RECT rc;
    GetClientRect(hwndDlg, &rc);
    HWND hScroll = CreateWindowExW(WS_EX_CONTROLPARENT, L"EliteDynScrollArea", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, 0, rc.right, rc.bottom, hwndDlg, NULL, g_hInstance, NULL);
    if (hPlaceholder) DestroyWindow(hPlaceholder);
    return hScroll;
}

struct MonitorInfo {
    int index;
    HMONITOR hMonitor;
    RECT rect;
};
std::vector<MonitorInfo> g_Monitors;
BOOL CALLBACK TaskbarPropsMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorInfo info;
    info.index = (int)g_Monitors.size();
    info.hMonitor = hMonitor;
    info.rect = *lprcMonitor;
    g_Monitors.push_back(info);
    return TRUE;
}

void PopulateOrbComboBox(HWND hCombo);
void SelectOrbComboBox(HWND hCombo, DWORD id);
DWORD GetSelectedOrbID(HWND hCombo);

INT_PTR CALLBACK MultiMonSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hScroll = NULL;
    static ULONG_PTR gdiplusToken = 0;
    switch (uMsg) {

    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        if (!gdiplusToken) {
            GdiplusStartupInput gdiplusStartupInput;
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        }
        hScroll = CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
        if (g_Monitors.empty()) EnumDisplayMonitors(NULL, NULL, TaskbarPropsMonitorEnumProc, 0);
        
        int y = 5;
        HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
        
        HKEY hKey;
        RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);
        
        for (const auto& mon : g_Monitors) {
            WCHAR title[64];
            wsprintfW(title, L"Monitor %d (%dx%d)", mon.index, mon.rect.right - mon.rect.left, mon.rect.bottom - mon.rect.top);
            HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, 175, hScroll, NULL, g_hInstance, NULL);
            SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
            
            HWND hChk1 = CreateWindowExW(0, L"Button", L"System Tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TRAY + mon.index), g_hInstance, NULL);
            HWND hChk2 = CreateWindowExW(0, L"Button", L"Clock", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 120, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_CLOCK + mon.index), g_hInstance, NULL);
            HWND hChk3 = CreateWindowExW(0, L"Button", L"Task Buttons", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 40, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TBTN + mon.index), g_hInstance, NULL);
            
            SendMessageW(hChk1, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk2, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk3, WM_SETFONT, (WPARAM)hFont, 0);
            
            HWND hLblMode = CreateWindowExW(0, L"Static", L"Start Menu Mode:", WS_CHILD | WS_VISIBLE, 15, y + 65, 120, 15, hScroll, NULL, g_hInstance, NULL);
            HWND hLblTrig = CreateWindowExW(0, L"Static", L"Start Menu Trigger:", WS_CHILD | WS_VISIBLE, 15, y + 90, 120, 15, hScroll, NULL, g_hInstance, NULL);
            HWND hLblOrb = CreateWindowExW(0, L"Static", L"Start Orb Theme:", WS_CHILD | WS_VISIBLE, 15, y + 115, 120, 15, hScroll, NULL, g_hInstance, NULL);

            SendMessageW(hLblMode, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hLblTrig, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hLblOrb, WM_SETFONT, (WPARAM)hFont, 0);

            HWND hCmbMode = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 60, 100, 100, hScroll, (HMENU)(ID_BASE_SM_MODE + mon.index), g_hInstance, NULL);
            HWND hCmbTrig = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 85, 100, 100, hScroll, (HMENU)(ID_BASE_SM_TRIG + mon.index), g_hInstance, NULL);
            HWND hCmbOrb = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 110, 100, 100, hScroll, (HMENU)(ID_BASE_SM_ORB + mon.index), g_hInstance, NULL);
            HWND hPreview = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | WS_BORDER, 250, y + 100, 54, 54, hScroll, (HMENU)(ID_BASE_SM_PREV + mon.index), g_hInstance, NULL);

            SetWindowSubclass(hCmbMode, NoMouseWheelSubclassProc, 1, 0);
            SetWindowSubclass(hCmbTrig, NoMouseWheelSubclassProc, 1, 0);
            SetWindowSubclass(hCmbOrb, NoMouseWheelSubclassProc, 1, 0);

            SendMessageW(hCmbMode, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hCmbTrig, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hCmbOrb, WM_SETFONT, (WPARAM)hFont, 0);

            SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Native Injection");
            SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
            SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Elite Custom Menu");
            SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Standalone");

            SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Left Click");
            SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Middle Click");
            SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Win Key");

            PopulateOrbComboBox(hCmbOrb);
            
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

                DWORD mode = 0; cbData = sizeof(DWORD);
                wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&mode, &cbData) != ERROR_SUCCESS) {
                    cbData = sizeof(DWORD);
                    RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&mode, &cbData);
                }
                SendMessageW(hCmbMode, CB_SETCURSEL, mode, 0);

                DWORD trig = 0; cbData = sizeof(DWORD);
                wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&trig, &cbData) != ERROR_SUCCESS) {
                    cbData = sizeof(DWORD);
                    RegQueryValueExW(hKey, L"StartMenuTrigger", NULL, NULL, (LPBYTE)&trig, &cbData);
                }
                SendMessageW(hCmbTrig, CB_SETCURSEL, trig, 0);

                DWORD orb = 0; cbData = sizeof(DWORD);
                wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&orb, &cbData) == ERROR_SUCCESS) {
                    SelectOrbComboBox(hCmbOrb, orb);
                } else {
                    SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                    orb = IDB_START_ORB;
                }
                HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orb);
                SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
            } else {
                SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                HBITMAP hBitmap = LoadPngResourceAsHBITMAP(IDB_START_ORB);
                SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hCmbMode, CB_SETCURSEL, 0, 0);
                SendMessageW(hCmbTrig, CB_SETCURSEL, 0, 0);
            }
            
            y += 185;
        }
        if (hKey) RegCloseKey(hKey);
        
        SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE, 0, y, 200 };
        SetScrollInfo(hScroll, SB_VERT, &si, TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            
            int id = LOWORD(wParam);
            if (HIWORD(wParam) == CBN_SELCHANGE && id >= ID_BASE_SM_ORB && id < ID_BASE_SM_ORB + 32) {
                int monIndex = id - ID_BASE_SM_ORB;
                HWND hCombo = GetDlgItem(hScroll, id);
                int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    DWORD orbId = SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
                    HWND hPreview = GetDlgItem(hScroll, ID_BASE_SM_PREV + monIndex);
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orbId);
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
                    DWORD v1 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_TRAY + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v2 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_CLOCK + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v3 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_TBTN + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    
                    DWORD mode = SendMessageW(GetDlgItem(hScroll, ID_BASE_SM_MODE + mon.index), CB_GETCURSEL, 0, 0);
                    DWORD trig = SendMessageW(GetDlgItem(hScroll, ID_BASE_SM_TRIG + mon.index), CB_GETCURSEL, 0, 0);
                    DWORD orb = GetSelectedOrbID(GetDlgItem(hScroll, ID_BASE_SM_ORB + mon.index));

                    WCHAR val[64];
                    wsprintfW(val, L"EnableTray_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v1, sizeof(DWORD));
                    wsprintfW(val, L"EnableClock_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v2, sizeof(DWORD));
                    wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v3, sizeof(DWORD));
                    
                    wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                    wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                    wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&orb, sizeof(DWORD));
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


INT_PTR CALLBACK ToolbarsSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {

    case WM_INITDIALOG:
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
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
    psp[0].hInstance = g_hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCEW(IDD_TASKBAR_PROPS);
    psp[0].pfnDlgProc = TaskbarSettingsDlgProc;
    psp[0].pszTitle = L"Taskbar";
    hPage = CreatePropertySheetPageW(&psp[0]);
    if (hPage) pages.push_back(hPage);



    psp[2].dwSize = sizeof(PROPSHEETPAGEW);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = g_hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCEW(IDD_MULTIMON_PROPS);
    psp[2].pfnDlgProc = MultiMonSettingsDlgProc;
    psp[2].pszTitle = L"Multi-Monitor Components";
    hPage = CreatePropertySheetPageW(&psp[2]);
    if (hPage) pages.push_back(hPage);

    psp[3].dwSize = sizeof(PROPSHEETPAGEW);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = g_hInstance;
    psp[3].pszTemplate = MAKEINTRESOURCEW(IDD_NATIVE_PROPS);
    psp[3].pfnDlgProc = NativeSettingsDlgProc;
    psp[3].pszTitle = L"Native Settings";
    hPage = CreatePropertySheetPageW(&psp[3]);
    if (hPage) pages.push_back(hPage);

    psp[4].dwSize = sizeof(PROPSHEETPAGEW);
    psp[4].dwFlags = PSP_USETITLE;
    psp[4].hInstance = g_hInstance;
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
        psp[5].hInstance = g_hInstance;
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
    psh.hInstance = g_hInstance;
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

void SelectOrbComboBox(HWND hCombo, DWORD id) {
    int count = SendMessageW(hCombo, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        if ((DWORD)SendMessageW(hCombo, CB_GETITEMDATA, i, 0) == id) { SendMessageW(hCombo, CB_SETCURSEL, i, 0); break; }
    }
}

DWORD GetSelectedOrbID(HWND hCombo) {
    int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
    if (sel != CB_ERR) return SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
    return IDB_START_ORB;
}

void ShowSecretEverything(HWND hwndOwner) {
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_SECRET_EVERYTHING), hwndOwner, SecretDlgProc);
}

void ShowSecretDLLScanner(HWND hwndOwner) {
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_SECRET_DLLSCANNER), hwndOwner, SecretDlgProc);
}





