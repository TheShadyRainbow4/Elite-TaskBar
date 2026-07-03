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

EliteTaskbarConfig g_Config;

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
            }
        }
        RegCloseKey(hKey);
    }
    Logger::Log(g_Config.Mode == TaskbarMode::Replace ? L"Mode: Replace Native Shell" : L"Mode: Independent");
}

int EXCEPTION_EXECUTE_HANDLER_FUNC(unsigned int code, struct _EXCEPTION_POINTERS* ep) {
    Logger::LogError(L"The taskbar has encountered a fatal existence failure.", code);
    
    wchar_t msg[512];
    wsprintfW(msg, L"The taskbar has encountered a fatal existence failure.\nError Code: 0x%08X", code);
    MessageBoxW(NULL, msg, L"EliteTaskbar - Fatal Error", MB_ICONERROR | MB_OK);
    
    return EXCEPTION_EXECUTE_HANDLER;
}

void RunApplication(HINSTANCE hInstance) {
    // 3 & 4. Initialize global logging function & Bootstrapper logic
    Logger::Initialize();
    
    // Initialize COM and Common Controls
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_COOL_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES | ICC_PAGESCROLLER_CLASS;
    InitCommonControlsEx(&icex);
    
    // Check for /settings command line argument to launch Settings dialog directly
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 1; i < argc; i++) {
            if (_wcsicmp(argv[i], L"/settings") == 0) {
                ShowTaskbarProperties(NULL);
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
    
    BufferedPaintInit();

    if (TaskbarWindow::Initialize(hInstance)) {
        Logger::Log(L"EliteTaskbar window initialized successfully. Entering message loop.");
        TaskbarWindow::RunMessageLoop();
        TaskbarWindow::Cleanup();
    }
    
    StartButton::Cleanup();
    BufferedPaintUnInit();
    
    if (SUCCEEDED(hrCoInit)) {
        CoUninitialize();
    }
    
    Logger::Log(L"EliteTaskbar shutting down.");
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 5. Initialize memory leak detection tracking (_CrtSetDbgFlag) for debug builds.
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // 8. Add structured exception handling (__try / __except)
    __try {
        RunApplication(hInstance);
    } __except(EXCEPTION_EXECUTE_HANDLER_FUNC(GetExceptionCode(), GetExceptionInformation())) {
        // 9. Crash dialog implemented in filter function
        return -1;
    }

    return 0;
}
