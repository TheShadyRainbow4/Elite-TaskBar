#include <windows.h>
#include <shlwapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <gdiplus.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")

// Mock GetEliteRegistryRoot
HKEY GetEliteRegistryRoot() {
    return HKEY_CURRENT_USER;
}

std::wstring GetThemeDirectory() {
    HKEY hKey;
    wchar_t themePathVal[MAX_PATH] = {0};
    DWORD cbData = sizeof(themePathVal);
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegQueryValueExW(hKey, L"DesktopThemePath", NULL, NULL, (LPBYTE)themePathVal, &cbData);
        RegCloseKey(hKey);
    }
    
    if (wcslen(themePathVal) > 0) {
        std::wstring pathStr(themePathVal);
        if (PathIsDirectoryW(pathStr.c_str())) {
            return pathStr;
        } else {
            size_t lastSlash = pathStr.find_last_of(L'\\');
            if (lastSlash != std::wstring::npos) {
                return pathStr.substr(0, lastSlash);
            }
        }
    }
    
    wchar_t winDir[MAX_PATH];
    if (GetWindowsDirectoryW(winDir, MAX_PATH) > 0) {
        std::wstring fallback = std::wstring(winDir) + L"\\Web\\Wallpaper";
        if (PathFileExistsW(fallback.c_str())) {
            return fallback;
        }
    }
    return L"";
}

int main() {
    // Initialize GDI+
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    std::wstring dir = GetThemeDirectory();
    std::wcout << L"Theme Directory: " << dir << std::endl;

    if (dir.empty()) {
        std::wcout << L"Error: Directory is empty." << std::endl;
        return 1;
    }

    if (!PathFileExistsW(dir.c_str())) {
        std::wcout << L"Error: Directory does not exist on disk." << std::endl;
        return 1;
    }

    std::vector<std::wstring> images;
    const wchar_t* extensions[] = { L"\\*.jpg", L"\\*.png", L"\\*.bmp", L"\\*.jpeg" };
    for (const auto& ext : extensions) {
        std::wstring query = dir + ext;
        std::wcout << L"Querying: " << query << std::endl;
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(query.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::wstring fullPath = dir + L"\\" + fd.cFileName;
                std::wcout << L"  Found file: " << fd.cFileName << L" -> " << fullPath << std::endl;
                images.push_back(fullPath);
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }

    std::wcout << L"Total images found: " << images.size() << std::endl;

    if (!images.empty()) {
        std::sort(images.begin(), images.end());
        for (const auto& img : images) {
            Gdiplus::Bitmap bmp(img.c_str());
            if (bmp.GetLastStatus() == Gdiplus::Ok) {
                std::wcout << L"  Gdiplus Load OK: " << img << std::endl;
            } else {
                std::wcout << L"  Gdiplus Load FAILED (Status: " << bmp.GetLastStatus() << L"): " << img << std::endl;
            }
        }
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}
