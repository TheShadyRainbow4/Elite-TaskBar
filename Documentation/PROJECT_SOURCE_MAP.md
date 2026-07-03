# Elite-TaskBar Source Map & Documentation

This document serves as a high-level map and explanation for all files within the `SourceFiles` directory and the root directory. It explains what each file does, how the architecture works, and acts as a master reference to ensure context is never lost.

## Root Directory Scripts
- **build.ps1**: The master MSVC build script. It locates Visual Studio, copies `EliteTaskbar.ico` from the root into `Resources\elite_icon.ico`, compiles resources (`rc.exe`), and compiles the `SourceFiles` into `BuildOutput\x64` and `BuildOutput\x86`. It copies the final 64-bit EXE to the root and auto-commits the repository on success.
- **backup.ps1**: A utility script triggered by `build.ps1` to zip/cab the project into the `Backups` folder before compilation to prevent data loss.
- **install_prereqs.ps1**: Installs necessary Windows SDK and MSVC components required to build the project.

## Core Application Logic
- **main.cpp**: The application entry point (`WinMain`). Initializes COM, GDI+, creates the main `TaskbarWindow`, parses command-line arguments (like `/devmode`), and runs the primary message pump (Event Loop). It also manages single-instance mutextes and loads global configurations (like `TaskbarMode`).
- **Config.h**: Defines the `g_Config` global structure and enums (like `TaskbarMode::Independent` vs `TaskbarMode::Replace`) so that various UI components know how the shell is currently configured to operate.

## Taskbar Shell (The Main GUI)
- **TaskbarWindow.h / TaskbarWindow.cpp**: The absolute core of the project.
  - **Responsibilities**:
    - Generates the main taskbar window class and applies DWM Aero Glass (`DwmExtendFrameIntoClientArea`).
    - Spawns child windows (`StartButton`, `ReBarWindow32`, `TrayNotifyWnd`, `TrayClockWClass`, `TrayShowDesktopButtonWClass`).
    - In `Replace` mode, it sets a background timer to actively hide and move the native Windows taskbar off-screen to bypass UIPI (User Interface Privilege Isolation) constraints for standard users.
    - Handles the taskbar Right-Click Context Menu, accurately mapping native commands (like Task Manager, Show Desktop) either to the native shell (via `PostMessageW`) or providing native standalone fallbacks.
    - Listens for `WM_SETTINGCHANGE` (specifically "TraySettings") to dynamically restart the shell when the user changes modes.

## Start Button (The Orb)
- **StartButton.h / StartButton.cpp**: Handles the custom drawing, hover animations, and click events for the Start Button (Orb). 
  - **Responsibilities**:
    - Subclasses the button to intercept mouse tracking (`WM_MOUSEHOVER`, `WM_MOUSELEAVE`).
    - Renders the button using GDI+ (scaling image assets or drawing a custom glass orb).
    - Sends a command to `TaskbarWindow` to invoke the native start menu (`IDM_START_EXPLORER` or sending `CTRL+ESC` / `LWIN`).

## System Tray & Clock
- **ClockWidget.h / ClockWidget.cpp**: Implements the native clock display inside the system tray notification area.
  - **Responsibilities**:
    - Subclasses `TrayClockWClass`.
    - Uses a 1-second `WM_TIMER` to pull `GetLocalTime()`.
    - Renders the time and date using GDI+ with `StringAlignmentCenter` to perfectly replicate the Windows 7/10 dual-line clock.

## Settings & Properties Dialogs
- **TaskbarProperties.h / TaskbarProperties.cpp**: Implements the **Custom Settings** tabbed property sheet.
  - **Responsibilities**:
    - Replicates the native Windows properties look using `CreatePropertySheetPageW` and `PropertySheetW` (the tabbed applet UI).
    - Reads/Writes our custom settings to `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`.
    - Specifically handles the "Taskbar Mode" radio buttons.
    - Uses `PSN_APPLY` to broadcast changes to the shell so `TaskbarWindow` reloads.
- **resource.h**: The standard header that maps UI element IDs (like `IDC_MODE_REPLACE`, `IDD_TASKBAR_PROPS`, `IDM_TASKBAR_SETTINGS`) to integer constants.
- **resources.rc**: The Resource Compiler script. It defines the layout, coordinates, and text for the Property Sheet Dialogs (`IDD_TASKBAR_PROPS`, `IDD_STARTMENU_PROPS`) and binds the Application Icon.
- **app.manifest**: XML manifest embedded into the final EXE. Crucially requests `Microsoft.Windows.Common-Controls` version 6.0.0.0 to ensure buttons, tabs, and property sheets render with the modern visual style instead of the Windows 95 classic theme.

## Debugging & Utilities
- **Logger.h / Logger.cpp**: Provides the `Log()` and `LogW()` functions. It strictly appends logs to `C:\EliteSoftware\Logs\Taskbar.log` (falling back to the local directory if permissions fail). It is used to trace UIPI failures, HWND values, and startup states.

## Architecture Workflow
1. User launches `EliteTaskbar.exe`.
2. `main.cpp` starts, initializes GDI+, and checks registry for `TaskbarMode`.
3. `TaskbarWindow::Initialize` runs. If `Replace` mode is active, it actively hooks a timer to push `Shell_TrayWnd` (the native taskbar) off-screen (`-10000, -10000`) and issues `SW_HIDE`.
4. `TaskbarWindow` creates itself as an `HWND_TOPMOST` layered window at the bottom of the primary monitor.
5. Child components (`StartButton`, `ClockWidget`) initialize themselves inside the taskbar.
6. The user right-clicks -> "Properties" calls `desk.cpl` to manage native Windows settings.
7. The user right-clicks -> "Elite Taskbar Settings" calls `TaskbarProperties.cpp`, generating the custom Property Sheet. Changing mode restarts the shell instantly.
