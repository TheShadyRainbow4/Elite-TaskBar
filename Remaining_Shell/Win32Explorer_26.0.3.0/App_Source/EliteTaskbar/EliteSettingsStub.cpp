#include <windows.h>
#include <shlwapi.h>
#include <cpl.h>
#include <commctrl.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

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

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
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
            return 1; 
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
