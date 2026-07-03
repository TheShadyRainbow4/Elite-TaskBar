#include <windows.h>
#include <shellapi.h>
#include <string>

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
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
