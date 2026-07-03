#include "TaskbarProperties.h"
#include "resource.h"
#include <commctrl.h>
#include <vector>

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                    else SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                }
                
                dwValue = 0;
                if (RegQueryValueExW(hKey, L"TrayMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_SETCHECK, BST_CHECKED, 0);
                    else SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
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
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD mode = (SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TaskbarMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                
                DWORD trayMode = (SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TrayMode", 0, REG_DWORD, (const BYTE*)&trayMode, sizeof(DWORD));
                
                RegCloseKey(hKey);
            }
            // Notify shell of setting changes
            SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
            
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK StartMenuSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                if (RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_START_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
                    else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_START_COMBO, BM_SETCHECK, BST_CHECKED, 0);
                    else SendDlgItemMessageW(hwndDlg, IDC_START_OPENSHELL, BM_SETCHECK, BST_CHECKED, 0);
                } else {
                    SendDlgItemMessageW(hwndDlg, IDC_START_OPENSHELL, BM_SETCHECK, BST_CHECKED, 0);
                }
                RegCloseKey(hKey);
            }
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_ADDSTRING, 0, (LPARAM)L"Left Click Opens Shell");
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_ADDSTRING, 0, (LPARAM)L"Shift+Click Opens Native");
            SendDlgItemMessageW(hwndDlg, IDC_START_TRIGGER, CB_SETCURSEL, 0, 0);
        }
        return TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == EN_CHANGE || HIWORD(wParam) == CBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD mode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_START_NATIVE, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_START_COMBO, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 2;
                RegSetValueExW(hKey, L"StartMenuMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                RegCloseKey(hKey);
            }
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

    PropertySheetW(&psh);
}
