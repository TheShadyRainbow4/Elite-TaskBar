# EliteTaskbar - Progman Window Architecture Investigation Report

## 1. Observation
From the investigation of the EliteTaskbar codebase, several critical structures and code paths were identified:
- **Application Startup and Message Loop**:
  In `SourceFiles/main.cpp`, the application runs in a Single-Threaded Apartment (`COINIT_APARTMENTTHREADED`) and handles the main loop on the primary thread via `TaskbarWindow::RunMessageLoop()` (lines 155-160):
  ```cpp
  if (TaskbarWindow::Initialize(hInstance)) {
      Logger::Log(L"EliteTaskbar window initialized successfully. Entering message loop.");
      TaskbarWindow::RunMessageLoop();
      TaskbarWindow::Cleanup();
  }
  ```
- **Operational Mode Detection**:
  In `SourceFiles/main.cpp` (lines 20-33), `QueryOperationalMode()` parses `g_Config.Mode` (e.g. `TaskbarMode::Replace`).
- **Existing Taskbar Window Registration**:
  In `SourceFiles/TaskbarWindow.cpp` (lines 1741-1750), classes for the custom taskbar are registered, using `"Shell_TrayWnd"` in `Replace` mode or `"Elite_SecondaryTrayWnd"` in `Independent` mode.
- **Native Taskbar Hiding and Restoration**:
  In `SourceFiles/TaskbarWindow.cpp` (lines 1732-1738), if `g_Config.Mode == TaskbarMode::Replace`, the native taskbar and secondary tray windows are hidden:
  ```cpp
  if (g_Config.Mode == TaskbarMode::Replace) {
      setVis(g_hNativeTaskbar, false);
      HWND hSec = NULL;
      while ((hSec = FindWindowExW(NULL, hSec, L"Shell_SecondaryTrayWnd", NULL)) != NULL) {
          setVis(hSec, false);
      }
  }
  ```
  During cleanup in `TaskbarWindow::Cleanup()` (lines 2026-2038), the native taskbar(s) are shown again, or if they are missing, a new instance of `explorer.exe` is launched to restore the system state.

## 2. Logic Chain
Based on these observations:
1. **Thread Affinity**: Since `main.cpp` runs a standard, single-threaded message loop (via `GetMessageW`), any window created on the main thread will share the message queue. Registering and creating a custom `Progman` desktop window on this thread guarantees thread safety and seamless event handling without the complexity of spawning separate threads.
2. **Replacement Mode Execution**: The custom desktop `Progman` window is only needed when EliteTaskbar runs in shell `Replace` mode (`g_Config.Mode == TaskbarMode::Replace`).
3. **Coexistence and Safety**: In line with the project's native coexistence policies (Rule 5), if an existing `Progman` window is present (due to a running explorer instance), it should be hidden upon initialization (similar to `Shell_TrayWnd` hiding) and restored during `Cleanup` to ensure the system is left in a stable state if EliteTaskbar exits.
4. **Desktop Z-Order**: Natively, `Progman` is the bottom-most window in the window manager. Creating it as a borderless, virtual-screen-sized popup window and handling `WM_MOUSEACTIVATE` and `WM_WINDOWPOSCHANGING` ensures it behaves exactly like a native desktop background.
5. **Modularity**: Because `TaskbarWindow.cpp` already spans over 2,000 lines and is solely focused on taskbar layout and controls, adding desktop class registration, wallpaper drawing, and desktop icon list views would violate the Single Responsibility Principle. Therefore, a new standalone module (`DesktopWindow.h` / `DesktopWindow.cpp`) is the cleanest architectural solution.

## 3. Caveats
- This investigation assumes that `explorer.exe` is either not running as shell or its desktop components can be hidden programmatically without crashing the explorer shell. If Windows forces explorer to recreate `Progman` dynamically, we may need to actively monitor window creation and hide newly spawned explorer windows.
- Multi-monitor setups will have a single `Progman` window covering the entire virtual screen coordinates (`SM_CXVIRTUALSCREEN`, `SM_CYVIRTUALSCREEN`). This is standard for modern Windows and simplifies rendering, but alternative strategies (creating a separate desktop window per display) might be considered if distinct icon spaces per screen are desired.

## 4. Conclusion
We recommend implementing a new modular component, `DesktopWindow`, to register and manage the custom borderless, bottom-Z-order `"Progman"` window.

### Suggested File Additions

#### `SourceFiles/DesktopWindow.h`
```cpp
#pragma once
#include <windows.h>

namespace DesktopWindow {
    bool Initialize(HINSTANCE hInstance);
    void Cleanup(HINSTANCE hInstance);
    HWND GetHWND();
}
```

#### `SourceFiles/DesktopWindow.cpp` (Conceptual Design)
```cpp
#include "DesktopWindow.h"
#include "Config.h"
#include "Logger.h"

static HWND s_hProgman = NULL;
static HWND s_hNativeProgman = NULL;

LRESULT CALLBACK DesktopWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEACTIVATE:
            // Intercept mouse clicks to prevent this window from taking focus and rising
            return MA_NOACTIVATE;

        case WM_WINDOWPOSCHANGING: {
            // Force this window to always sit at the bottom of the Z-order
            WINDOWPOS* lpw = (WINDOWPOS*)lParam;
            lpw->hwndInsertAfter = HWND_BOTTOM;
            lpw->flags &= ~SWP_NOZORDER;
            break;
        }

        case WM_DESTROY:
            s_hProgman = NULL;
            break;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

namespace DesktopWindow {
    bool Initialize(HINSTANCE hInstance) {
        // Find existing desktop window (e.g. native Progman)
        s_hNativeProgman = FindWindowW(L"Progman", NULL);
        if (s_hNativeProgman) {
            Logger::Log(L"Existing Progman window detected. Hiding for shell replacement.");
            ShowWindow(s_hNativeProgman, SW_HIDE);
        }

        // Register class
        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DesktopWndProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = L"Progman";
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

        if (!RegisterClassExW(&wc)) {
            Logger::Log(L"Failed to register custom Progman window class.");
            return false;
        }

        // Retrieve full virtual screen geometry (encompassing all displays)
        int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
        int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
        int cx = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        int cy = GetSystemMetrics(SM_CYVIRTUALSCREEN);

        // Create the borderless window with WS_EX_TOOLWINDOW style
        s_hProgman = CreateWindowExW(
            WS_EX_TOOLWINDOW,
            L"Progman", L"",
            WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            x, y, cx, cy,
            NULL, NULL, hInstance, NULL
        );

        if (!s_hProgman) {
            Logger::Log(L"Failed to create custom Progman window.");
            UnregisterClassW(L"Progman", hInstance);
            return false;
        }

        // Enforce bottom Z-order positioning
        SetWindowPos(s_hProgman, HWND_BOTTOM, x, y, cx, cy, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        Logger::Log(L"Custom Progman window initialized at the bottom of the Z-order.");
        return true;
    }

    void Cleanup(HINSTANCE hInstance) {
        if (s_hProgman) {
            DestroyWindow(s_hProgman);
            s_hProgman = NULL;
        }
        UnregisterClassW(L"Progman", hInstance);

        // Restore native desktop window if hidden
        if (s_hNativeProgman && IsWindow(s_hNativeProgman)) {
            Logger::Log(L"Restoring native Progman window.");
            ShowWindow(s_hNativeProgman, SW_SHOW);
        }
    }

    HWND GetHWND() {
        return s_hProgman;
    }
}
```

### Integration Plan
1. **Initialize**: Call `DesktopWindow::Initialize(hInstance)` inside `TaskbarWindow::Initialize` if `g_Config.Mode == TaskbarMode::Replace` is active.
2. **Cleanup**: Call `DesktopWindow::Cleanup(hInstance)` at the beginning of `TaskbarWindow::Cleanup` to safely tear down the custom desktop and restore the native one.

## 5. Verification Method
1. **Build Process Verification**:
   Verify compilation using `build.ps1` to ensure new files (`DesktopWindow.h`, `DesktopWindow.cpp`) compile correctly into the main executable targets:
   ```powershell
   .\build.ps1
   ```
2. **Execution Inspection**:
   Run EliteTaskbar in `Replace` mode and check the debug log file at `%SystemDrive%\EliteSoftware\Logs\EliteTaskbar.log` for successful initialization/destruction entries.
3. **Z-order and Event Verification**:
   - Use Spy++ or standard Win32 window scanners to verify a window of class `"Progman"` is active, resides at the bottom of the Z-order, and has size matching the virtual screen dimensions.
   - Click on the custom desktop; confirm focus does not shift to the desktop window (remains with the active application window), verified by Spy++ window focus messages.
   - Exit the taskbar (via tray icon right-click context menu "Exit EliteTaskbar"); verify the custom window is destroyed and the native desktop is successfully restored.
