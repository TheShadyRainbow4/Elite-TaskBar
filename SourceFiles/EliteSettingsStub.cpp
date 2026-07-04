#include <windows.h>
#include <shlwapi.h>
#include <cpl.h>
#include "resource_stub.h"
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")

void LaunchSettings(HINSTANCE hInst) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hInst, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    PathAppendW(path, L"EliteTaskbar.exe");
    
    ShellExecuteW(NULL, L"open", path, L"/settings", NULL, SW_SHOWNORMAL);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    LaunchSettings(hInstance);
    return 0;
}

extern "C" __declspec(dllexport) LONG APIENTRY CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2) {
    static HINSTANCE hInstance = NULL;
    switch (uMsg) {
        case CPL_INIT:
            hInstance = GetModuleHandleW(NULL);
            return TRUE;
        case CPL_GETCOUNT:
            return 1;
        case CPL_INQUIRE: {
            LPCPLINFO info = (LPCPLINFO)lParam2;
            info->idIcon = IDI_STUB_ICON;
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
            info->hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_STUB_ICON));
            wcscpy_s(info->szName, L"Elite Taskbar");
            wcscpy_s(info->szInfo, L"Configure Elite Taskbar Multi-Monitor features and Start Menu integrations.");
            wcscpy_s(info->szHelpFile, L"");
            return 1; // Return 1 to indicate we handled CPL_NEWINQUIRE
        }
        case CPL_DBLCLK:
            LaunchSettings(hInstance);
            return 0;
        case CPL_STOP:
        case CPL_EXIT:
            return 0;
    }
    return 0;
}
