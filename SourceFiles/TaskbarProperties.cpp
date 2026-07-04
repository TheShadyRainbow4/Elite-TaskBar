#include "TaskbarProperties.h"
#include "resource.h"
#include "Logger.h"
#include <commctrl.h>
#include <vector>
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

void UpdateOrbPreview(HWND hwndDlg, DWORD orbId) {
    HRSRC hResInfo = FindResourceW(GetModuleHandle(NULL), MAKEINTRESOURCEW(orbId), (LPCWSTR)RT_RCDATA);
    if (!hResInfo) return;
    HGLOBAL hResData = LoadResource(GetModuleHandle(NULL), hResInfo);
    if (!hResData) return;
    DWORD size = SizeofResource(GetModuleHandle(NULL), hResInfo);
    void* pData = LockResource(hResData);
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal) return;
    void* pGlobalData = GlobalLock(hGlobal);
    memcpy(pGlobalData, pData, size);
    GlobalUnlock(hGlobal);
    IStream* pStream = NULL;
    if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) == S_OK) {
        Bitmap* bmp = Bitmap::FromStream(pStream);
        if (bmp && bmp->GetLastStatus() == Ok) {
            UINT width = bmp->GetWidth();
            UINT height = bmp->GetHeight();
            int numFrames = ((height * 100) / width > 300) ? 4 : 3;
            UINT sliceHeight = height / numFrames;

            Bitmap* frame = bmp->Clone(0, 0, width, sliceHeight, PixelFormat32bppARGB);
            if (frame) {
                HBITMAP hBmp = NULL;
                frame->GetHBITMAP(Color(255, 255, 255, 255), &hBmp);
                if (hBmp) {
                    HBITMAP hOld = (HBITMAP)SendDlgItemMessageW(hwndDlg, IDC_ORB_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
                    if (hOld) DeleteObject(hOld);
                }
                delete frame;
            }
            delete bmp;
        }
        pStream->Release();
    }
}

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        Logger::Log(L"TaskbarSettingsDlgProc WM_INITDIALOG entered");
        {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                    else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
                    else SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
                }
                
                dwValue = 0;
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TrayMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_SETCHECK, BST_CHECKED, 0);
                    else SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
                }
                
                dwValue = 0;
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarButtonWidthMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_FIXED, BM_SETCHECK, BST_CHECKED, 0);
                    else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_ICONS, BM_SETCHECK, BST_CHECKED, 0);
                    else SendDlgItemMessageW(hwndDlg, IDC_WIDTH_AUTO, BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    SendDlgItemMessageW(hwndDlg, IDC_WIDTH_AUTO, BM_SETCHECK, BST_CHECKED, 0);
                }
                
                dwValue = 0;
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"NativeRegistryMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_NATIVE_REGISTRY_MODE, BM_SETCHECK, BST_CHECKED, 0);
                }

                dwValue = 0;
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarHoverPreview", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_HOVER_PREVIEW, BM_SETCHECK, BST_CHECKED, 0);
                }
                RegCloseKey(hKey);
            }
            
            // Read native settings
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_SETCHECK, BST_CHECKED, 0); // 0 = Locked
                } else {
                    SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_SETCHECK, BST_CHECKED, 0);
                }
                
                dwValue = 0;
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_SETCHECK, BST_CHECKED, 0);
                }
                RegCloseKey(hKey);
            }
            
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                BYTE blob[64];
                DWORD cbData = sizeof(blob);
                if (RegQueryValueExW(hKey, L"Settings", NULL, NULL, blob, &cbData) == ERROR_SUCCESS) {
                    if (cbData >= 9) {
                        if (blob[8] & 0x01) {
                            SendDlgItemMessageW(hwndDlg, IDC_AUTOHIDE_TASKBAR, BM_SETCHECK, BST_CHECKED, 0);
                        }
                    }
                }
                RegCloseKey(hKey);
            }
            SendDlgItemMessageW(hwndDlg, IDC_MONITOR_LIST, CB_ADDSTRING, 0, (LPARAM)L"All Monitors");
            SendDlgItemMessageW(hwndDlg, IDC_MONITOR_LIST, CB_ADDSTRING, 0, (LPARAM)L"Primary Monitor");
            SendDlgItemMessageW(hwndDlg, IDC_MONITOR_LIST, CB_SETCURSEL, 0, 0);
            
            SendDlgItemMessageW(hwndDlg, IDC_COMPONENTS_LIST, LB_ADDSTRING, 0, (LPARAM)L"Start Button");
            SendDlgItemMessageW(hwndDlg, IDC_COMPONENTS_LIST, LB_ADDSTRING, 0, (LPARAM)L"Taskband (Icons)");
            SendDlgItemMessageW(hwndDlg, IDC_COMPONENTS_LIST, LB_ADDSTRING, 0, (LPARAM)L"Notification Area");
            SendDlgItemMessageW(hwndDlg, IDC_COMPONENTS_LIST, LB_ADDSTRING, 0, (LPARAM)L"Clock");
            SendDlgItemMessageW(hwndDlg, IDC_COMPONENTS_LIST, LB_ADDSTRING, 0, (LPARAM)L"Show Desktop Button");
        }
        return TRUE;

    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == LBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;

    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            // Apply changes
            HKEY hKey;
            
            DWORD nativeSync = (SendDlgItemMessageW(hwndDlg, IDC_NATIVE_REGISTRY_MODE, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;

            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD mode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 2;
                RegSetValueExW(hKey, L"TaskbarMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                
                DWORD trayMode = (SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TrayMode", 0, REG_DWORD, (const BYTE*)&trayMode, sizeof(DWORD));
                
                DWORD widthMode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_WIDTH_FIXED, BM_GETCHECK, 0, 0) == BST_CHECKED) widthMode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_WIDTH_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) widthMode = 2;
                RegSetValueExW(hKey, L"TaskbarButtonWidthMode", 0, REG_DWORD, (const BYTE*)&widthMode, sizeof(DWORD));
                
                RegSetValueExW(hKey, L"NativeRegistryMode", 0, REG_DWORD, (const BYTE*)&nativeSync, sizeof(DWORD));
                
                DWORD hoverPreview = (SendDlgItemMessageW(hwndDlg, IDC_HOVER_PREVIEW, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TaskbarHoverPreview", 0, REG_DWORD, (const BYTE*)&hoverPreview, sizeof(DWORD));

                RegCloseKey(hKey);
            }
            
            // Native keys
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD locked = (SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 0 : 1; // 0 = Locked
                RegSetValueExW(hKey, L"TaskbarSizeMove", 0, REG_DWORD, (const BYTE*)&locked, sizeof(DWORD));
                
                DWORD smallIcons = (SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TaskbarSmallIcons", 0, REG_DWORD, (const BYTE*)&smallIcons, sizeof(DWORD));
                RegCloseKey(hKey);
            }
            
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                BYTE blob[64];
                DWORD cbData = sizeof(blob);
                if (RegQueryValueExW(hKey, L"Settings", NULL, NULL, blob, &cbData) == ERROR_SUCCESS && cbData >= 9) {
                    if (SendDlgItemMessageW(hwndDlg, IDC_AUTOHIDE_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                        blob[8] |= 0x01;
                    } else {
                        blob[8] &= ~0x01;
                    }
                    RegSetValueExW(hKey, L"Settings", 0, REG_BINARY, blob, cbData);
                }
                RegCloseKey(hKey);
            }

            // Notify shell of setting changes
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

struct SettingsMonitorData {
    int count;
};

BOOL CALLBACK SettingsMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    SettingsMonitorData* data = (SettingsMonitorData*)dwData;
    data->count++;
    return TRUE;
}

INT_PTR CALLBACK StartMenuSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) != ERROR_SUCCESS) {
                    dwValue = 0;
                }
                
                dwValue = IDB_START_ORB;
                cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"StartOrbID", NULL, NULL, (LPBYTE)&dwValue, &cbData) != ERROR_SUCCESS) {
                    dwValue = IDB_START_ORB;
                }
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"Default Orb");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 0, IDB_START_ORB);
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"1Orb");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 1, IDB_START_ORB_1ORB);
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"Aqua Bottom");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 2, IDB_START_ORB_AQUABOTTOM);
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"Dunes");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 3, IDB_START_ORB_DUNES);
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"Indigo");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 4, IDB_START_ORB_INDIGO);
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"Sapphire");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 5, IDB_START_ORB_SAPPHIRE);
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"Uranus");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 6, IDB_START_ORB_URANUS);
                
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_ADDSTRING, 0, (LPARAM)L"Vienna Bottom");
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETITEMDATA, 7, IDB_START_ORB_VIENNABOTTOM);
                
                int selIndex = 0;
                int count = SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETCOUNT, 0, 0);
                for (int i = 0; i < count; i++) {
                    if (SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETITEMDATA, i, 0) == dwValue) {
                        selIndex = i;
                        break;
                    }
                }
                SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETCURSEL, selIndex, 0);
                
                RegCloseKey(hKey);
                UpdateOrbPreview(hwndDlg, dwValue);
            }
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Menu (Default)");
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Menu (Shift for Native)");
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_ADDSTRING, 0, (LPARAM)L"Native Menu (Shift for Open-Shell)");
            
            // dwValue might have been read from the registry above; if not, we must read it here
            DWORD modeValue = 0;
            HKEY hKeyTrigger;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyTrigger) == ERROR_SUCCESS) {
                DWORD cbData = sizeof(DWORD);
                RegQueryValueExW(hKeyTrigger, L"StartMenuMode", NULL, NULL, (LPBYTE)&modeValue, &cbData);
                RegCloseKey(hKeyTrigger);
            }
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_SETCURSEL, modeValue, 0);
            
            DWORD currentMode = 0;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD cbData = sizeof(DWORD);
                RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&currentMode, &cbData);
                RegCloseKey(hKey);
            }
            if (currentMode > 3) currentMode = 0;
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_SETCURSEL, currentMode, 0);
            
            SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_ADDSTRING, 0, (LPARAM)L"Global Configuration");
            SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_SETITEMDATA, 0, -1);
            SettingsMonitorData monData = {0};
            EnumDisplayMonitors(NULL, NULL, SettingsMonitorEnumProc, (LPARAM)&monData);
            for (int i = 0; i < monData.count; i++) {
                WCHAR szMon[32];
                wsprintfW(szMon, L"Monitor %d", i);
                SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_ADDSTRING, 0, (LPARAM)szMon);
                SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_SETITEMDATA, i + 1, i);
            }
            SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_SETCURSEL, 0, 0);
        }
        return TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_START_MONITOR_LIST) {
            int monSel = SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_GETCURSEL, 0, 0);
            if (monSel != CB_ERR) {
                int monIndex = SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_GETITEMDATA, monSel, 0);
                HKEY hKey;
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    DWORD dwValue = IDB_START_ORB;
                    DWORD cbData = sizeof(DWORD);
                    bool found = false;
                    if (monIndex >= 0) {
                        WCHAR valueName[64];
                        wsprintfW(valueName, L"StartOrbID_Mon%d", monIndex);
                        if (RegQueryValueExW(hKey, valueName, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                            found = true;
                        }
                    }
                    if (!found) {
                        if (RegQueryValueExW(hKey, L"StartOrbID", NULL, NULL, (LPBYTE)&dwValue, &cbData) != ERROR_SUCCESS) {
                            dwValue = IDB_START_ORB;
                        }
                    }
                    
                    int count = SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETCOUNT, 0, 0);
                    int selIndex = 0;
                    for (int i = 0; i < count; i++) {
                        if (SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETITEMDATA, i, 0) == dwValue) {
                            selIndex = i;
                            break;
                        }
                    }
                    SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_SETCURSEL, selIndex, 0);
                    RegCloseKey(hKey);
                    UpdateOrbPreview(hwndDlg, dwValue);
                }
            }
        }
        else if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            
            if (LOWORD(wParam) == IDC_ORB_SELECTOR && HIWORD(wParam) == CBN_SELCHANGE) {
                HKEY hKey;
                if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                    int selIndex = SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETCURSEL, 0, 0);
                    DWORD orbId = 0;
                    if (selIndex != CB_ERR) {
                        orbId = (DWORD)SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETITEMDATA, selIndex, 0);
                        int monSel = SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_GETCURSEL, 0, 0);
                        int monIndex = (monSel != CB_ERR) ? SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_GETITEMDATA, monSel, 0) : -1;
                        if (monIndex >= 0) {
                            WCHAR valueName[64];
                            wsprintfW(valueName, L"StartOrbID_Mon%d", monIndex);
                            RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                        } else {
                            RegSetValueExW(hKey, L"StartOrbID", 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                        }
                    }
                    RegCloseKey(hKey);
                    if (orbId != 0) UpdateOrbPreview(hwndDlg, orbId);
                }
                SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
            }
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD mode = SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_GETCURSEL, 0, 0);
                if (mode == CB_ERR) mode = 0;
                RegSetValueExW(hKey, L"StartMenuMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                
                int selIndex = SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETCURSEL, 0, 0);
                if (selIndex != CB_ERR) {
                    DWORD orbId = (DWORD)SendDlgItemMessageW(hwndDlg, IDC_ORB_SELECTOR, CB_GETITEMDATA, selIndex, 0);
                    UpdateOrbPreview(hwndDlg, orbId);
                    
                    int monSel = SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_GETCURSEL, 0, 0);
                    int monIndex = (monSel != CB_ERR) ? SendDlgItemMessageW(hwndDlg, IDC_START_MONITOR_LIST, CB_GETITEMDATA, monSel, 0) : -1;
                    
                    if (monIndex >= 0) {
                        WCHAR valueName[64];
                        wsprintfW(valueName, L"StartOrbID_Mon%d", monIndex);
                        RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                    } else {
                        RegSetValueExW(hKey, L"StartOrbID", 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                        for (int i = 0; i < 32; i++) {
                            WCHAR valueName[64];
                            wsprintfW(valueName, L"StartOrbID_Mon%d", i);
                            RegDeleteValueW(hKey, valueName);
                        }
                    }
                }
                
                RegCloseKey(hKey);
            }
            
            // Notify taskbars to update orb
            EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
                WCHAR className[256];
                if (GetClassNameW(hwnd, className, 256) && (wcscmp(className, L"TrayNotifyWnd") == 0 || wcscmp(className, L"StartButtonWClass") == 0)) { // Need a way to notify StartButton to reload. Actually, StartButton uses WM_SETTINGCHANGE
                     // Let's just broadcast WM_SETTINGCHANGE
                }
                return TRUE;
            }, 0);
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
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
        {
            SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Address");
            SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Links");
            SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Desktop");
        }
        return TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED && LOWORD(wParam) == IDC_TOOLBAR_NEW) {
            MessageBoxW(hwndDlg, L"Folder browser dialog will appear here.", L"New Toolbar", MB_OK);
            return TRUE;
        }
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == LBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK GenericPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

void ShowTaskbarProperties(HWND hwndOwner) {
    Logger::Log(L"ShowTaskbarProperties called");
    std::vector<HPROPSHEETPAGE> pages;
    PROPSHEETPAGEW psp[6] = {0};

    HPROPSHEETPAGE hPage;
    
    // 0: Taskbar
    psp[0].dwSize = sizeof(PROPSHEETPAGEW);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = GetModuleHandle(NULL);
    psp[0].pszTemplate = MAKEINTRESOURCEW(IDD_TASKBAR_PROPS);
    psp[0].pfnDlgProc = TaskbarSettingsDlgProc;
    psp[0].pszTitle = L"Taskbar";
    Logger::Log(L"Creating Taskbar Page...");
    hPage = CreatePropertySheetPageW(&psp[0]);
    if (hPage) pages.push_back(hPage);

    // 1: Start Menu
    psp[1].dwSize = sizeof(PROPSHEETPAGEW);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = GetModuleHandle(NULL);
    psp[1].pszTemplate = MAKEINTRESOURCEW(IDD_STARTMENU_PROPS);
    psp[1].pfnDlgProc = StartMenuSettingsDlgProc;
    psp[1].pszTitle = L"Start Menu";
    hPage = CreatePropertySheetPageW(&psp[1]);
    if (hPage) pages.push_back(hPage);

    // 2: Desktop
    psp[2].dwSize = sizeof(PROPSHEETPAGEW);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = GetModuleHandle(NULL);
    psp[2].pszTemplate = MAKEINTRESOURCEW(IDD_DESKTOP_PROPS);
    psp[2].pfnDlgProc = GenericPageDlgProc;
    psp[2].pszTitle = L"Desktop";
    hPage = CreatePropertySheetPageW(&psp[2]);
    if (hPage) pages.push_back(hPage);

    // 3: Toolbars
    psp[3].dwSize = sizeof(PROPSHEETPAGEW);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = GetModuleHandle(NULL);
    psp[3].pszTemplate = MAKEINTRESOURCEW(IDD_TOOLBARS_PROPS);
    psp[3].pfnDlgProc = ToolbarsSettingsDlgProc;
    psp[3].pszTitle = L"Toolbars";
    hPage = CreatePropertySheetPageW(&psp[3]);
    if (hPage) pages.push_back(hPage);

    // Authentication
    bool showDebugTabs = false;
    LPWSTR cmdLine = GetCommandLineW();
    if (wcsstr(cmdLine, L"/devmode")) showDebugTabs = true;
    
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue = 0;
        DWORD cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"EnableDebugTabs", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
            if (dwValue == 1) showDebugTabs = true;
        }
        RegCloseKey(hKey);
    }

    if (showDebugTabs) {
        psp[4].dwSize = sizeof(PROPSHEETPAGEW);
        psp[4].dwFlags = PSP_USETITLE;
        psp[4].hInstance = GetModuleHandle(NULL);
        psp[4].pszTemplate = MAKEINTRESOURCEW(IDD_SECRET_EVERYTHING);
        psp[4].pfnDlgProc = GenericPageDlgProc;
        psp[4].pszTitle = L"Everything Indexer";
        hPage = CreatePropertySheetPageW(&psp[4]);
        if (hPage) pages.push_back(hPage);

        psp[5].dwSize = sizeof(PROPSHEETPAGEW);
        psp[5].dwFlags = PSP_USETITLE;
        psp[5].hInstance = GetModuleHandle(NULL);
        psp[5].pszTemplate = MAKEINTRESOURCEW(IDD_SECRET_DLLSCANNER);
        psp[5].pfnDlgProc = GenericPageDlgProc;
        psp[5].pszTitle = L"DLL Scanner";
        hPage = CreatePropertySheetPageW(&psp[5]);
        if (hPage) pages.push_back(hPage);
    }

    // Determine starting tab
    int startPage = 0;
    if (wcsstr(cmdLine, L"/tab:startmenu")) startPage = 1;
    else if (wcsstr(cmdLine, L"/tab:desktop")) startPage = 2;
    else if (wcsstr(cmdLine, L"/tab:toolbars")) startPage = 3;

    if (pages.empty()) {
        MessageBoxW(hwndOwner, L"Failed to load any property sheet pages.", L"Error", MB_ICONERROR);
        return;
    }

    PROPSHEETHEADERW psh = { sizeof(PROPSHEETHEADERW) };
    psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID;
    psh.hwndParent = hwndOwner;
    psh.hInstance = GetModuleHandle(NULL);
    psh.pszIcon = MAKEINTRESOURCEW(IDI_PREFERENCES);
    psh.pszCaption = L"Taskbar and Start Menu Properties";
    psh.nPages = (UINT)pages.size();
    psh.nStartPage = startPage;
    psh.phpage = pages.data();

    Logger::Log(L"Calling PropertySheetW...");
    psh.phpage = pages.data();
    
    PropertySheetW(&psh);
    Logger::Log(L"Property sheet closed.");
}

INT_PTR CALLBACK SecretDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hwndDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }
    return FALSE;
}

void ShowSecretEverything(HWND hwndOwner) {
    DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_SECRET_EVERYTHING), hwndOwner, SecretDlgProc);
}

void ShowSecretDLLScanner(HWND hwndOwner) {
    DialogBoxW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_SECRET_DLLSCANNER), hwndOwner, SecretDlgProc);
}
