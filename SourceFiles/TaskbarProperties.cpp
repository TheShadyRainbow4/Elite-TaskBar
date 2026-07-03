#include "TaskbarProperties.h"
#include "resource.h"
#include <commctrl.h>

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

void ShowTaskbarProperties(HWND hwndOwner) {
    PROPSHEETPAGEW psp = { sizeof(PROPSHEETPAGEW) };
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = GetModuleHandle(NULL);
    psp.pszTemplate = MAKEINTRESOURCEW(IDD_TASKBAR_PROPS);
    psp.pfnDlgProc = TaskbarSettingsDlgProc;
    psp.pszTitle = L"Taskbar";

    PROPSHEETHEADERW psh = { sizeof(PROPSHEETHEADERW) };
    psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID | PSH_PROPSHEETPAGE;
    psh.hwndParent = hwndOwner;
    psh.hInstance = GetModuleHandle(NULL);
    psh.pszIcon = MAKEINTRESOURCEW(IDI_PREFERENCES);
    psh.pszCaption = L"Taskbar and Start Menu Properties";
    psh.nPages = 1;
    psh.nStartPage = 0;
    psh.ppsp = &psp;

    PropertySheetW(&psh);
}
