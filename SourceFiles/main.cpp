#include <windows.h>
#include <crtdbg.h>
#include "Config.h"
#include "Logger.h"
#include "TaskbarWindow.h"
#include "StartButton.h"
#include "TaskbarProperties.h"
#include "resource.h"
#include <commctrl.h>
#include <uxtheme.h>
#include <tlhelp32.h>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

EliteTaskbarConfig g_Config = { L"", TaskbarMode::Independent, ButtonWidthMode::Auto, TrayOverflowMode::Win7Flyout, 1, false, false, true, 0, true, true, true, {} };

// - Draftsman-Dan
bool IsExplorerRunning() {
    bool running = false;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32W pe = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(hSnapshot, &pe)) {
            do {
                if (_wcsicmp(pe.szExeFile, L"explorer.exe") == 0) {
                    running = true;
                    break;
                }
            } while (Process32NextW(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    return running;
}
HINSTANCE g_hInstance = NULL;

void QueryOperationalMode() {
    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, KEY_READ | KEY_WRITE, &hKey); // - Draftsman-Dan
    if (status != ERROR_SUCCESS) {
        status = RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL); // - Draftsman-Dan
    }
    g_Config.Mode = TaskbarMode::SecondaryOnly; // Default
    bool modeFound = false;
    DWORD dwValue = 0;
    if (status == ERROR_SUCCESS) {
        DWORD bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            modeFound = true;
        }
    }
    if (!modeFound) {
        // - Draftsman-Dan
        if (IsExplorerRunning()) {
            dwValue = 2; // SecondaryOnly
            g_Config.Mode = TaskbarMode::SecondaryOnly;
        } else {
            dwValue = 1; // Replace
            g_Config.Mode = TaskbarMode::Replace;
        }
        if (status == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"TaskbarMode", 0, REG_DWORD, (const BYTE*)&dwValue, sizeof(DWORD));
        }
    } else {
        if (dwValue == 1) {
            g_Config.Mode = TaskbarMode::Replace;
        } else if (dwValue == 2) {
            g_Config.Mode = TaskbarMode::SecondaryOnly;
        } else {
            g_Config.Mode = TaskbarMode::Independent;
        }
    }
    if (status == ERROR_SUCCESS) {
        DWORD bufferSize = sizeof(DWORD);
        
        dwValue = 0;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TaskbarButtonWidthMode", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            if (dwValue == 1) {
                g_Config.ButtonWidth = ButtonWidthMode::Fixed;
            } else if (dwValue == 2) {
                g_Config.ButtonWidth = ButtonWidthMode::IconsOnly;
            } else {
                g_Config.ButtonWidth = ButtonWidthMode::Auto;
            }
        }
        
        dwValue = 1;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TaskbarButtonFixedWidthSize", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.FixedWidthSize = dwValue;
        } else {
            g_Config.FixedWidthSize = 1;
        }
        
        dwValue = 0;
        bufferSize = sizeof(DWORD);
        g_Config.OverflowMode = TrayOverflowMode::Win7Flyout;
        if (RegQueryValueExW(hKey, L"TrayMode", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            if (dwValue == 1) {
                g_Config.OverflowMode = TrayOverflowMode::VistaInline;
            }
        }
        
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TaskbarHoverPreview", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.ShowPreviews = (dwValue == 1);
        }
        
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"UseNativeTaskBand", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.UseNativeTaskBand = (dwValue == 1);
        }
        
        dwValue = 1;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"EnableTwoRowTray", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.EnableTwoRowTray = (dwValue == 1);
        } else {
            g_Config.EnableTwoRowTray = true;
        }

        // Load ManualTrayWidth setting - Builder-Bob
        dwValue = 0;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"ManualTrayWidth", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.ManualTrayWidth = (int)dwValue;
        } else {
            g_Config.ManualTrayWidth = 0;
        }

        // Load EnableEliteTaskbar setting - Builder-Bob
        dwValue = 1;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"EnableEliteTaskbar", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.EnableEliteTaskbar = (dwValue == 1);
        } else {
            g_Config.EnableEliteTaskbar = true;
        }

        // Load DynamicClockWidth setting - Draftsman-Dan
        dwValue = 1;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"DynamicClockWidth", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.DynamicClockWidth = (dwValue == 1);
        } else {
            g_Config.DynamicClockWidth = true;
        }

        // Load HorizontalTrayChevron setting - Draftsman-Dan
        dwValue = 1;
        bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"HorizontalTrayChevron", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            g_Config.HorizontalTrayChevron = (dwValue == 1);
        } else {
            g_Config.HorizontalTrayChevron = true;
        }
        
        RegCloseKey(hKey);
    }
    const wchar_t* modeStr = L"Mode: Independent";
    if (g_Config.Mode == TaskbarMode::Replace) modeStr = L"Mode: Replace Native Shell";
    else if (g_Config.Mode == TaskbarMode::SecondaryOnly) modeStr = L"Mode: Secondary Monitors Only";
    Logger::Log(modeStr);
}

int EXCEPTION_EXECUTE_HANDLER_FUNC(unsigned int code, struct _EXCEPTION_POINTERS* ep) {
    Logger::LogError(L"The taskbar has encountered a fatal existence failure.", code);
    
    wchar_t msg[512];
    wsprintfW(msg, L"The taskbar has encountered a fatal existence failure.\nError Code: 0x%08X", code);
        MessageBoxW(NULL, msg, L"EliteTaskbar - Fatal Error", MB_ICONERROR | MB_OK);
    
    return EXCEPTION_EXECUTE_HANDLER;
}

void PerformBootSynchronization() {
    HKEY hMasterKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Master", 0, KEY_READ, &hMasterKey) != ERROR_SUCCESS) {
        return;
    }
    
    HKEY hAdvancedKey = NULL;
    RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Settings", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hAdvancedKey, NULL); // - Draftsman-Dan
    
    DWORD dwIndex = 0;
    wchar_t valueName[16384];
    DWORD cbValueName = 16384;
    DWORD dwType = 0;
    BYTE valData[16384];
    DWORD cbValData = 16384;
    
    while (RegEnumValueW(hMasterKey, dwIndex, valueName, &cbValueName, NULL, &dwType, valData, &cbValData) == ERROR_SUCCESS) {
        if (hAdvancedKey) {
            RegSetValueExW(hAdvancedKey, valueName, 0, dwType, valData, cbValData);
        }
        
        // Sync to native Windows registry paths
        if (_wcsicmp(valueName, L"Hidden") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hNative, L"Hidden", 0, dwType, valData, cbValData);
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"HideFileExt") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hNative, L"HideFileExt", 0, dwType, valData, cbValData);
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"DwmAnimationsEnabled") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\WindowMetrics", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                DWORD val = *(DWORD*)valData;
                const wchar_t* strVal = val ? L"1" : L"0";
                RegSetValueExW(hNative, L"MinAnimate", 0, REG_SZ, (const BYTE*)strVal, (DWORD)(wcslen(strVal) + 1) * sizeof(wchar_t));
                RegCloseKey(hNative);
            }
            ANIMATIONINFO ai = { sizeof(ANIMATIONINFO) };
            ai.iMinAnimate = *(DWORD*)valData;
            SystemParametersInfoW(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &ai, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        }
        else if (_wcsicmp(valueName, L"DwmGlassEnabled") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hNative, L"Composition", 0, dwType, valData, cbValData);
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"DwmBorderSize") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\WindowMetrics", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                wchar_t strVal[32];
                swprintf_s(strVal, L"%lu", *(DWORD*)valData);
                RegSetValueExW(hNative, L"BorderWidth", 0, REG_SZ, (const BYTE*)strVal, (DWORD)(wcslen(strVal) + 1) * sizeof(wchar_t));
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"DesktopIconsEnabled") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                DWORD val = *(DWORD*)valData;
                DWORD hideVal = val ? 0 : 1;
                RegSetValueExW(hNative, L"HideIcons", 0, REG_DWORD, (const BYTE*)&hideVal, sizeof(DWORD));
                RegCloseKey(hNative);
            }
        }
        
        dwIndex++;
        cbValueName = 16384;
        cbValData = 16384;
    }
    
    if (hMasterKey) RegCloseKey(hMasterKey);
    if (hAdvancedKey) RegCloseKey(hAdvancedKey);
    
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
}

void RunApplication(HINSTANCE hInstance) {
    // Parse command line for -restartPid - Draftsman-Dan
    int argcRest = 0;
    LPWSTR* argvRest = CommandLineToArgvW(GetCommandLineW(), &argcRest);
    if (argvRest) {
        for (int i = 1; i < argcRest - 1; i++) {
            if (_wcsicmp(argvRest[i], L"-restartPid") == 0) {
                DWORD pid = (DWORD)_wtoi(argvRest[i + 1]);
                if (pid != 0) {
                    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
                    if (hProcess) {
                        WaitForSingleObject(hProcess, 5000);
                        CloseHandle(hProcess);
                    }
                }
                break;
            }
        }
        LocalFree(argvRest);
    }

    // 3 & 4. Initialize global logging function & Bootstrapper logic
    Logger::Initialize();
    PerformBootSynchronization();
    
    // Initialize COM and Common Controls
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES | ICC_PAGESCROLLER_CLASS;
    InitCommonControlsEx(&icex);
    
    // UIPI Bypass: Allow lower-elevation applications to send messages
    for (UINT msg = 0; msg <= 0xFFFF; ++msg) {
        ChangeWindowMessageFilter(msg, MSGFLT_ADD);
    }
    
    // Check for /settings command line argument to launch Settings dialog directly
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (_wcsicmp(argv[i], L"/settings") == 0 || _wcsicmp(argv[i], L"-settings") == 0) {
                ShowTaskbarProperties(NULL);
                CoUninitialize();
                LocalFree(argv);
                return;
            } else if (_wcsicmp(argv[i], L"/everything") == 0 || _wcsicmp(argv[i], L"-everything") == 0) {
                ShowSecretEverything(NULL);
                CoUninitialize();
                LocalFree(argv);
                return;
            } else if (_wcsicmp(argv[i], L"/dllscanner") == 0 || _wcsicmp(argv[i], L"-dllscanner") == 0) {
                ShowSecretDLLScanner(NULL);
                CoUninitialize();
                LocalFree(argv);
                return;
            }
        }
    }

    bool allowMultiple = false;
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (_wcsicmp(argv[i], L"-allowMultiple") == 0) allowMultiple = true;
        }
        LocalFree(argv);
    }
    
    HANDLE hMutex = NULL;
    if (!allowMultiple) {
        hMutex = CreateMutexW(NULL, TRUE, L"EliteTaskbar_Instance_Mutex");
        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            Logger::Log(L"EliteTaskbar is already running. Exiting.");
            return;
        }
    }
    
    // Initialize COM for Legacy Clock UI
    HRESULT hrCoInit = CoInitialize(NULL);
    
    // 6 & 7. Query Windows Registry & Declare global struct
    QueryOperationalMode();
    
    // Initialize default toolbars registry configuration - Builder-Bob
    HKEY hToolbarsKey = NULL;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\EliteSoftware\\Win32Explorer\\Toolbars", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hToolbarsKey, NULL) == ERROR_SUCCESS) {
        std::wstring qlPath = L"%APPDATA%\\Microsoft\\Internet Explorer\\Quick Launch";
        RegSetValueExW(hToolbarsKey, L"Quick Launch", 0, REG_SZ, (const BYTE*)qlPath.c_str(), (DWORD)(qlPath.length() + 1) * sizeof(wchar_t));
        RegCloseKey(hToolbarsKey);
    }
    
    BufferedPaintInit();
    
    ULONG_PTR gdiToken = 0;
    StartButton::GlobalInitialize(gdiToken);

    if (TaskbarWindow::Initialize(hInstance)) {
        Logger::Log(L"EliteTaskbar window initialized successfully. Entering message loop.");
        TaskbarWindow::RunMessageLoop();
        TaskbarWindow::Cleanup();
    }
    
    StartButton::GlobalCleanup(gdiToken);
    BufferedPaintUnInit();
    
    if (SUCCEEDED(hrCoInit)) {
        CoUninitialize();
    }
    
    Logger::Log(L"EliteTaskbar shutting down.");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    g_hInstance = hInstance;
    // 5. Initialize memory leak detection tracking (_CrtSetDbgFlag) for debug builds.
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // [Command Line Import Option] - Builder-Bob
    if (__argc >= 3 && _wcsicmp(__wargv[1], L"/import") == 0) {
        ImportSettingsFromXMLPathSilently(__wargv[2]);
        NotifySettingsChange();
        return 0;
    }

    // 8. Add structured exception handling (__try / __except)
    __try {
        RunApplication(hInstance);
    } __except(EXCEPTION_EXECUTE_HANDLER_FUNC(GetExceptionCode(), GetExceptionInformation())) {
        // 9. Crash dialog implemented in filter function
        return -1;
    }

    return 0;
}
