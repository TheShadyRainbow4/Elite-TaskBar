#include "TaskbarProperties.h"
#include "resource.h"
#include <commctrl.h>
#include <vector>

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        // Set up checkboxes based on current registry settings
        {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD dwValue = 0;
                DWORD cbData = sizeof(DWORD);
                
                // Lock Taskbar
                if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_SETCHECK, (dwValue == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
                }
                
                // Small Icons
                if (RegQueryValueExW(hKey, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                    SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_SETCHECK, (dwValue == 1) ? BST_CHECKED : BST_UNCHECKED, 0);
                }
                
                RegCloseKey(hKey);
            }
        }
        return TRUE;

    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            // Apply changes
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                
                DWORD locked = (SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 0 : 1;
                RegSetValueExW(hKey, L"TaskbarSizeMove", 0, REG_DWORD, (const BYTE*)&locked, sizeof(DWORD));
                
                DWORD smallIcons = (SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TaskbarSmallIcons", 0, REG_DWORD, (const BYTE*)&smallIcons, sizeof(DWORD));
                
                RegCloseKey(hKey);
                
                // Notify shell of setting changes
                SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
            }
            
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

    // 0: Taskbar
    psp[0].dwSize = sizeof(PROPSHEETPAGEW);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = GetModuleHandle(NULL);
    psp[0].pszTemplate = MAKEINTRESOURCEW(IDD_TASKBAR_PROPS);
    psp[0].pfnDlgProc = TaskbarSettingsDlgProc;
    psp[0].pszTitle = L"Taskbar";
    pages.push_back(CreatePropertySheetPageW(&psp[0]));

    // 1: Start Menu
    psp[1].dwSize = sizeof(PROPSHEETPAGEW);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = GetModuleHandle(NULL);
    psp[1].pszTemplate = MAKEINTRESOURCEW(IDD_STARTMENU_PROPS);
    psp[1].pfnDlgProc = GenericPageDlgProc;
    psp[1].pszTitle = L"Start Menu";
    pages.push_back(CreatePropertySheetPageW(&psp[1]));

    // 2: Desktop
    psp[2].dwSize = sizeof(PROPSHEETPAGEW);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = GetModuleHandle(NULL);
    psp[2].pszTemplate = MAKEINTRESOURCEW(IDD_DESKTOP_PROPS);
    psp[2].pfnDlgProc = GenericPageDlgProc;
    psp[2].pszTitle = L"Desktop";
    pages.push_back(CreatePropertySheetPageW(&psp[2]));

    // 3: Toolbars
    psp[3].dwSize = sizeof(PROPSHEETPAGEW);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = GetModuleHandle(NULL);
    psp[3].pszTemplate = MAKEINTRESOURCEW(IDD_TOOLBARS_PROPS);
    psp[3].pfnDlgProc = GenericPageDlgProc;
    psp[3].pszTitle = L"Toolbars";
    pages.push_back(CreatePropertySheetPageW(&psp[3]));

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
        pages.push_back(CreatePropertySheetPageW(&psp[4]));

        psp[5].dwSize = sizeof(PROPSHEETPAGEW);
        psp[5].dwFlags = PSP_USETITLE;
        psp[5].hInstance = GetModuleHandle(NULL);
        psp[5].pszTemplate = MAKEINTRESOURCEW(IDD_SECRET_DLLSCANNER);
        psp[5].pfnDlgProc = GenericPageDlgProc;
        psp[5].pszTitle = L"DLL Scanner";
        pages.push_back(CreatePropertySheetPageW(&psp[5]));
    }

    // Determine starting tab
    int startPage = 0;
    if (wcsstr(cmdLine, L"/tab:startmenu")) startPage = 1;
    else if (wcsstr(cmdLine, L"/tab:desktop")) startPage = 2;
    else if (wcsstr(cmdLine, L"/tab:toolbars")) startPage = 3;

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
