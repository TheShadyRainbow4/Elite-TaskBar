#include <windows.h>
#include <shlwapi.h>
#include <cpl.h>
#include <commctrl.h>
#include <comdef.h>
#include <msxml6.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

extern void ShowTaskbarProperties(HWND hwndOwner);
HINSTANCE g_hInstance = NULL;

#ifdef _WINDLL
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        g_hInstance = hModule;
    }
    return TRUE;
}
#endif

void LaunchSettings(HINSTANCE hInst, HWND hwndOwner) {
    if (hInst != NULL) g_hInstance = hInst;
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES | ICC_USEREX_CLASSES;
    InitCommonControlsEx(&icex);
    
    ShowTaskbarProperties(hwndOwner);
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    // UIPI Bypass: Allow lower-elevation applications to send messages
    for (UINT msg = 0; msg <= 0xFFFF; ++msg) {
        ChangeWindowMessageFilter(msg, MSGFLT_ADD);
    }
    
    // Build the path to System32
    wchar_t sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);

    LaunchSettings(hInstance, NULL);
    return 0;
}

extern "C" __declspec(dllexport) LONG APIENTRY CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2) {
    switch (uMsg) {
        case CPL_INIT:
            if (g_hInstance == NULL) {
                GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&CPlApplet, &g_hInstance);
            }
            return TRUE;
        case CPL_GETCOUNT:
            return 1;
        case CPL_INQUIRE: {
            LPCPLINFO info = (LPCPLINFO)lParam2;
            info->idIcon = 101; 
            info->idName = 0;
            info->idInfo = 0;
            info->lData = 0;
            return 0;
        }
        case CPL_NEWINQUIRE: {
            LPNEWCPLINFOW info = (LPNEWCPLINFOW)lParam2;
            info->dwSize = sizeof(NEWCPLINFOW);
            info->dwFlags = 0;
            info->dwHelpContext = 0;
            info->lData = 0;
            info->hIcon = LoadIconW(g_hInstance, MAKEINTRESOURCEW(101));
            wcscpy_s(info->szName, L"Elite Taskbar");
            wcscpy_s(info->szInfo, L"Configure Elite Taskbar");
            wcscpy_s(info->szHelpFile, L"");
            return 0; 
        }
        case CPL_DBLCLK:
            LaunchSettings(g_hInstance, hwndCPl);
            return 0;
        case CPL_STOP:
        case CPL_EXIT:
            return 0;
    }
    return 0;
}

// ImportSettingsW — called elevated via:
//   rundll32 EliteSettings.cpl,ImportSettingsW <config.xml>
// Parses the XML, reads settings, and writes them as DWORDs to
// HKLM\Software\EliteSoftware\Win32Explorer\Settings.
extern "C" __declspec(dllexport) void CALLBACK ImportSettingsW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    if (!lpszCmdLine || !*lpszCmdLine)
        return;

    // --- Extract the config.xml path from the command line ---
    // Strip leading/trailing whitespace and optional surrounding quotes.
    WCHAR szPath[MAX_PATH];
    LPWSTR pSrc = lpszCmdLine;
    while (*pSrc == L' ' || *pSrc == L'\t') pSrc++;

    BOOL bQuoted = FALSE;
    if (*pSrc == L'"') {
        bQuoted = TRUE;
        pSrc++;
    }

    WCHAR *pDst = szPath;
    WCHAR *pEnd = szPath + MAX_PATH - 1;
    while (*pSrc && pDst < pEnd) {
        if (bQuoted && *pSrc == L'"') break;
        if (!bQuoted && (*pSrc == L' ' || *pSrc == L'\t')) break;
        *pDst++ = *pSrc++;
    }
    *pDst = L'\0';

    if (!szPath[0])
        return;

    // --- Initialize COM ---
    HRESULT hrInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hrInit) && hrInit != RPC_E_CHANGED_MODE)
        return;

    // Ensure CoUninitialize is called on every exit path.
    BOOL bNeedUninit = SUCCEEDED(hrInit);

    do { // Scope block for cleanup via break

        // --- Load the XML document ---
        IXMLDOMDocument2 *pDoc = NULL;
        HRESULT hr = CoCreateInstance(
            __uuidof(DOMDocument60), NULL, CLSCTX_INPROC_SERVER,
            __uuidof(IXMLDOMDocument2), (void**)&pDoc);
        if (FAILED(hr) || !pDoc)
            break;

        pDoc->put_async(VARIANT_FALSE);
        pDoc->put_validateOnParse(VARIANT_FALSE);
        pDoc->put_resolveExternals(VARIANT_FALSE);

        // Set XPath as the selection language (required for attribute selectors).
        pDoc->setProperty(_bstr_t(L"SelectionLanguage"), _variant_t(L"XPath"));

        VARIANT_BOOL vbSuccess = VARIANT_FALSE;
        hr = pDoc->load(_variant_t(szPath), &vbSuccess);
        if (FAILED(hr) || vbSuccess != VARIANT_TRUE) {
            pDoc->Release();
            break;
        }

        // --- Open (or create) the target registry key ---
        HKEY hKey = NULL;
        LONG lRes = RegCreateKeyExW(
            HKEY_LOCAL_MACHINE,
            L"Software\\EliteSoftware\\Win32Explorer\\Settings",
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE, NULL, &hKey, NULL);
        if (lRes != ERROR_SUCCESS || !hKey) {
            pDoc->Release();
            break;
        }

        // --- Settings table ---
        // Each entry: XML @name, registry value name, is-boolean (yes/no → 1/0).
        struct SettingEntry {
            LPCWSTR xmlName;
            LPCWSTR regName;
            BOOL    bBoolean; // TRUE = yes/no, FALSE = integer
        };

        static const SettingEntry settings[] = {
            { L"EnableEliteTaskbar",       L"EnableEliteTaskbar",       TRUE  },
            { L"EnableEliteStartMenu",     L"EnableEliteStartMenu",     TRUE  },
            { L"EnableDefaultGroupByType", L"EnableDefaultGroupByType", TRUE  },
            { L"EnableNativeViewMode",     L"EnableNativeViewMode",     TRUE  },
            { L"EnableShellBagsSupport",   L"EnableShellBagsSupport",   TRUE  },
            { L"ReplaceExplorerMode",      L"ReplaceExplorerMode",      FALSE },
            { L"ShowHiddenFiles",          L"ShowHiddenFiles",          TRUE  },
            { L"ShowExtensions",           L"ShowExtensions",           TRUE  },
        };

        // --- Query each setting and write to registry (HKLM + HKCU) ---
        for (int i = 0; i < _countof(settings); i++) {
            // Build XPath: /ExplorerPlusPlus/Settings/Setting[@name='<name>']
            WCHAR szXPath[256];
            _snwprintf_s(szXPath, _countof(szXPath), _TRUNCATE,
                L"/ExplorerPlusPlus/Settings/Setting[@name='%s']",
                settings[i].xmlName);

            IXMLDOMNode *pNode = NULL;
            hr = pDoc->selectSingleNode(_bstr_t(szXPath), &pNode);
            if (FAILED(hr) || !pNode)
                continue;

            BSTR bstrText = NULL;
            hr = pNode->get_text(&bstrText);
            pNode->Release();

            if (FAILED(hr) || !bstrText)
                continue;

            DWORD dwValue = 0;

            if (settings[i].bBoolean) {
                // "yes" → 1, anything else → 0
                if (_wcsicmp(bstrText, L"yes") == 0)
                    dwValue = 1;
                else
                    dwValue = 0;
            } else {
                // Integer value (e.g. ReplaceExplorerMode 0-2)
                dwValue = (DWORD)_wtoi(bstrText);
            }

            SysFreeString(bstrText);

            // Write to HKLM
            RegSetValueExW(hKey, settings[i].regName, 0, REG_DWORD,
                (const BYTE*)&dwValue, sizeof(DWORD));
        }

        // --- Also write all settings to HKCU for user-level persistence ---
        HKEY hKeyUser = NULL;
        LONG lResUser = RegCreateKeyExW(
            HKEY_CURRENT_USER,
            L"Software\\EliteSoftware\\Win32Explorer\\Settings",
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE, NULL, &hKeyUser, NULL);
        if (lResUser == ERROR_SUCCESS && hKeyUser) {
            for (int i = 0; i < _countof(settings); i++) {
                WCHAR szXPath[256];
                _snwprintf_s(szXPath, _countof(szXPath), _TRUNCATE,
                    L"/ExplorerPlusPlus/Settings/Setting[@name='%s']",
                    settings[i].xmlName);

                IXMLDOMNode *pNode = NULL;
                hr = pDoc->selectSingleNode(_bstr_t(szXPath), &pNode);
                if (FAILED(hr) || !pNode)
                    continue;

                BSTR bstrText = NULL;
                hr = pNode->get_text(&bstrText);
                pNode->Release();
                if (FAILED(hr) || !bstrText)
                    continue;

                DWORD dwValue = 0;
                if (settings[i].bBoolean) {
                    dwValue = (_wcsicmp(bstrText, L"yes") == 0) ? 1 : 0;
                } else {
                    dwValue = (DWORD)_wtoi(bstrText);
                }
                SysFreeString(bstrText);

                RegSetValueExW(hKeyUser, settings[i].regName, 0, REG_DWORD,
                    (const BYTE*)&dwValue, sizeof(DWORD));
            }
            RegCloseKey(hKeyUser);
        }

        RegCloseKey(hKey);
        pDoc->Release();

    } while (0);

    if (bNeedUninit)
        CoUninitialize();
}
