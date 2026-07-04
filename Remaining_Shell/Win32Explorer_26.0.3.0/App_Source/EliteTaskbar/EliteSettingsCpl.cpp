#include <windows.h>
#include <cpl.h>

void RunEmbeddedExe(HINSTANCE hInstance) {
    HRSRC hRes = FindResource(hInstance, MAKEINTRESOURCE(1), RT_RCDATA);
    if (!hRes) return;
    HGLOBAL hData = LoadResource(hInstance, hRes);
    if (!hData) return;
    DWORD size = SizeofResource(hInstance, hRes);
    void* pData = LockResource(hData);
    
    WCHAR tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    WCHAR exePath[MAX_PATH];
    GetTempFileNameW(tempPath, L"EST", 0, exePath);
    
    // Rename extension to .exe
    wcscpy_s(exePath + wcslen(exePath) - 4, 5, L".exe");
    
    HANDLE hFile = CreateFileW(exePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE) {
        DWORD written;
        WriteFile(hFile, pData, size, &written, NULL);
        CloseHandle(hFile);
        
        WCHAR cplPath[MAX_PATH];
        GetModuleFileNameW(hInstance, cplPath, MAX_PATH);
        WCHAR* lastSlash = wcsrchr(cplPath, L'\\');
        if (lastSlash) *lastSlash = L'\0';
        
        WCHAR params[MAX_PATH + 10];
        wsprintfW(params, L"\"%s\"", cplPath);

        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"open";
        sei.lpFile = exePath;
        sei.lpParameters = params;
        sei.nShow = SW_SHOWNORMAL;
        sei.lpFile = exePath;
        sei.nShow = SW_SHOWNORMAL;
        if (ShellExecuteExW(&sei)) {
            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);
        }
        DeleteFileW(exePath);
    }
}

extern "C" __declspec(dllexport) LONG APIENTRY CPlApplet(HWND hwndCPl, UINT uMsg, LPARAM lParam1, LPARAM lParam2) {
    switch (uMsg) {
        case CPL_INIT:
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
            
            HMODULE hMod = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&CPlApplet, &hMod);
            info->hIcon = LoadIconW(hMod, MAKEINTRESOURCEW(101));
            
            wcscpy_s(info->szName, L"Elite Taskbar");
            wcscpy_s(info->szInfo, L"Configure Elite Taskbar Multi-Monitor features and settings.");
            wcscpy_s(info->szHelpFile, L"");
            return 1; 
        }
        case CPL_DBLCLK: {
            HMODULE hMod = NULL;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)&CPlApplet, &hMod);
            RunEmbeddedExe(hMod);
            return 0;
        }
        case CPL_STOP:
        case CPL_EXIT:
            return 0;
    }
    return 0;
}
