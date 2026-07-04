#include "stdafx.h"
#pragma warning(disable: 4100 4244 4267 4189 4312)
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

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

EliteTaskbarConfig g_Config = { L"", TaskbarMode::Independent, ButtonWidthMode::Auto, TrayOverflowMode::Win7Flyout, false, false, {} };
HINSTANCE g_hInstance = NULL;

void QueryOperationalMode() {
    HKEY hKey;
    LSTATUS status = RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);
    g_Config.Mode = TaskbarMode::Independent; // Default
    if (status == ERROR_SUCCESS) {
        DWORD dwValue = 0;
        DWORD bufferSize = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &bufferSize) == ERROR_SUCCESS) {
            if (dwValue == 1) {
                g_Config.Mode = TaskbarMode::Replace;
            } else if (dwValue == 2) {
                g_Config.Mode = TaskbarMode::SecondaryOnly;
            }
        }
        
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

ULONG_PTR g_gdiToken = 0;
HRESULT g_hrCoInit = E_FAIL;

void InitializeTaskbar(HINSTANCE hInstance) {
    // 3 & 4. Initialize global logging function & Bootstrapper logic
    Logger::Initialize();
    
    // Initialize COM and Common Controls
    g_hrCoInit = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES | ICC_PAGESCROLLER_CLASS;
    InitCommonControlsEx(&icex);
    
    // Check for /settings command line argument to launch Settings dialog directly
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (_wcsicmp(argv[i], L"/settings") == 0 || _wcsicmp(argv[i], L"-settings") == 0) {
                ShowTaskbarProperties(NULL);
                if (SUCCEEDED(g_hrCoInit)) CoUninitialize();
                LocalFree(argv);
                return;
            } else if (_wcsicmp(argv[i], L"/everything") == 0 || _wcsicmp(argv[i], L"-everything") == 0) {
                ShowSecretEverything(NULL);
                if (SUCCEEDED(g_hrCoInit)) CoUninitialize();
                LocalFree(argv);
                return;
            } else if (_wcsicmp(argv[i], L"/dllscanner") == 0 || _wcsicmp(argv[i], L"-dllscanner") == 0) {
                ShowSecretDLLScanner(NULL);
                if (SUCCEEDED(g_hrCoInit)) CoUninitialize();
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
    
    // 6 & 7. Query Windows Registry & Declare global struct
    QueryOperationalMode();
    
    BufferedPaintInit();
    
    StartButton::GlobalInitialize(g_gdiToken);

    if (TaskbarWindow::Initialize(hInstance)) {
        Logger::Log(L"EliteTaskbar window initialized successfully. Native message loop will take over.");
    }
}

void CleanupTaskbar() {
    TaskbarWindow::Cleanup();
    StartButton::GlobalCleanup(g_gdiToken);
    BufferedPaintUnInit();
    
    if (SUCCEEDED(g_hrCoInit)) {
        CoUninitialize();
    }
    
    Logger::Log(L"EliteTaskbar shutting down.");
}



