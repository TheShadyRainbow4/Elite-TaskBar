#pragma once
#include <windows.h>
#include <string>
#include <vector>

enum class TaskbarMode {
    Independent,
    Replace,
    SecondaryOnly
};

enum class ButtonWidthMode {
    Auto,
    Fixed,
    IconsOnly
};

enum class TrayOverflowMode {
    VistaInline,
    Win7Flyout
};

struct EliteTaskbarConfig {
    std::wstring Theme;
    TaskbarMode Mode;
    ButtonWidthMode ButtonWidth;
    TrayOverflowMode OverflowMode;
    int FixedWidthSize;
    bool ShowPreviews;
    bool UseNativeTaskBand;
    std::vector<HMONITOR> MonitorTargets;
};

extern EliteTaskbarConfig g_Config;
void QueryOperationalMode();

inline HKEY GetEliteRegistryRoot() {
    HKEY hKey;
    DWORD dwValue = 0;
    DWORD cbData = sizeof(DWORD);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"EnablePortableMirror", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue == 1) {
            RegCloseKey(hKey);
            return HKEY_LOCAL_MACHINE;
        }
        RegCloseKey(hKey);
    }
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"EnablePortableMirror", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue == 1) {
            RegCloseKey(hKey);
            return HKEY_LOCAL_MACHINE;
        }
        RegCloseKey(hKey);
    }
    return HKEY_CURRENT_USER;
}
