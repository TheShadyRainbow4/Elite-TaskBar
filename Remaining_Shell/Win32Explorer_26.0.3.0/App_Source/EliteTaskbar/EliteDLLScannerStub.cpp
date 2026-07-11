#include <windows.h>
#include <shellapi.h>
#include <string>

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow) {
    for (UINT msg = 0; msg <= 0xFFFF; ++msg) {
        ChangeWindowMessageFilter(msg, MSGFLT_ADD);
    }
    
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    std::wstring exePath(path);
    size_t pos = exePath.find_last_of(L"\\/");
    if (pos != std::wstring::npos) {
        std::wstring targetPath = exePath.substr(0, pos) + L"\\EliteTaskbar.exe";
        ShellExecuteW(NULL, L"open", targetPath.c_str(), L"-dllscanner", NULL, SW_SHOWNORMAL);
    }
    return 0;
}
