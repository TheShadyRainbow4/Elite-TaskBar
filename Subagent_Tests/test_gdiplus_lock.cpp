#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include <gdiplus.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")

bool IsFileLocked(const wchar_t* path) {
    HANDLE hFile = CreateFileW(path, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        return true;
    }
    CloseHandle(hFile);
    return false;
}

int main() {
    ULONG_PTR token;
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartup(&token, &input, NULL);

    const wchar_t* img1 = L"C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\Subagent_Tests\\test_theme\\img1.jpg";
    const wchar_t* img2 = L"C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\Subagent_Tests\\test_theme\\img2.jpg";

    std::wcout << L"Initial: img1 locked: " << IsFileLocked(img1) << L", img2 locked: " << IsFileLocked(img2) << std::endl;

    Gdiplus::Bitmap* bmp = new Gdiplus::Bitmap(img1);
    std::wcout << L"After load img1: img1 locked: " << IsFileLocked(img1) << L", img2 locked: " << IsFileLocked(img2) << std::endl;

    delete bmp;
    std::wcout << L"After delete img1: img1 locked: " << IsFileLocked(img1) << L", img2 locked: " << IsFileLocked(img2) << std::endl;

    Gdiplus::Bitmap* bmp2 = new Gdiplus::Bitmap(img2);
    std::wcout << L"After load img2: img1 locked: " << IsFileLocked(img1) << L", img2 locked: " << IsFileLocked(img2) << std::endl;

    delete bmp2;
    std::wcout << L"After delete img2: img1 locked: " << IsFileLocked(img1) << L", img2 locked: " << IsFileLocked(img2) << std::endl;

    Gdiplus::GdiplusShutdown(token);
    return 0;
}
