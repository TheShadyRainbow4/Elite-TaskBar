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
    bool EnableTwoRowTray;
    int ManualTrayWidth; // Manual width in pixels - Builder-Bob
    bool EnableEliteTaskbar; // Toggle to skip taskbar instantiation - Builder-Bob
    bool DynamicClockWidth; // - Draftsman-Dan
    bool HorizontalTrayChevron; // - Draftsman-Dan
    std::vector<HMONITOR> MonitorTargets;
};

extern EliteTaskbarConfig g_Config;
void QueryOperationalMode();

inline HKEY GetEliteRegistryRoot() {
    return HKEY_LOCAL_MACHINE;
}

// Mirror a DWORD setting to both HKLM and HKCU so settings are always preserved
inline void SaveEliteSettingDWORD(LPCWSTR subKey, LPCWSTR valueName, DWORD value) {
    HKEY hKey = NULL;
    // Write to HKLM (primary)
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
    // Mirror to HKCU (user-level backup)
    hKey = NULL;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

// Mirror a string setting to both HKLM and HKCU
inline void SaveEliteSettingString(LPCWSTR subKey, LPCWSTR valueName, LPCWSTR value) {
    HKEY hKey = NULL;
    DWORD cbData = (DWORD)(wcslen(value) + 1) * sizeof(WCHAR);
    // Write to HKLM (primary)
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE*)value, cbData);
        RegCloseKey(hKey);
    }
    // Mirror to HKCU (user-level backup)
    hKey = NULL;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, subKey, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName, 0, REG_SZ, (const BYTE*)value, cbData);
        RegCloseKey(hKey);
    }
}
