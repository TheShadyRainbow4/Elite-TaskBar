# Elite-TaskBar Source Map & Documentation

This document serves as a high-level map and explanation for all files within the `SourceFiles` directory and the root directory. It explains what each file does, how the architecture works, and acts as a master reference to ensure context is never lost.

## Root Directory Scripts
- **build.ps1**: The master MSVC build script. It locates Visual Studio, copies icons, compiles resources (`rc.exe`), and compiles the `SourceFiles` into `BuildOutput\x64` and `BuildOutput\x86`. It handles building both the main `EliteTaskbar.exe` and the `EliteSettings.exe` stub executable, auto-committing the repository on success.
- **backup.ps1**: A utility script triggered by `build.ps1` to zip/cab the project into the `Backups` folder before compilation to prevent data loss.
- **install_prereqs.ps1**: Installs necessary Windows SDK and MSVC components required to build the project.
- **verify_final_polish.ps1**: Programmatically validates the application of Desktop Background, Quick Launch, 2-Row Tray, and Clock Seconds toggles across both normal (HKCU) and Portable Mirror (HKLM & XML) modes.

## E2E Testing & Verification Scripts
- **Subagent_Tests/run_comprehensive_e2e.ps1**: Executes E2E verification of 10 features across 4 tiers (Feature, Boundary, Pairwise, Scenario) utilizing non-disruptive, single-shell lifecycle dispatch.

## Core Application Logic
- **main.cpp**: The application entry point (`WinMain`). Initializes COM, GDI+, creates the main `TaskbarWindow`, handles the `/settings` argument to launch the custom property sheet, parses command-line arguments (like `-allowMultiple`), and runs the primary message pump (Event Loop). It also manages single-instance mutexes and loads global configurations.
- **Config.h**: Defines the `g_Config` global structure and enums (like `TaskbarMode::Independent` vs `TaskbarMode::Replace`) so that various UI components know how the shell is currently configured to operate.

## Taskbar Shell (The Main GUI)
- **TaskbarWindow.h / TaskbarWindow.cpp**: The absolute core of the project.
  - **Responsibilities**:
    - Generates the main taskbar window class and applies DWM Aero Glass (`DwmExtendFrameIntoClientArea`).
    - Spawns child windows (`StartButton`, `ReBarWindow32`, `TrayNotifyWnd`, `TrayClockWClass`, `TrayShowDesktopButtonWClass`).
    - In `Replace` mode, it sets a background timer to actively hide and move the native Windows taskbar off-screen to bypass UIPI (User Interface Privilege Isolation) constraints for standard users.
    - Handles the taskbar Right-Click Context Menu, accurately mapping native commands (like Task Manager, Show Desktop) either to the native shell (via `PostMessageW` sending native command IDs like 410, 401) or providing native standalone fallbacks.
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
- **TrayIconScraper.h / TrayIconScraper.cpp**: Implements system tray notification icon scraping and replication toolbar drawing.
  - **Responsibilities**:
    - Scrapes tray notification icons from both the visible tray (`Shell_TrayWnd`) and hidden overflow tray (`NotifyIconOverflowWindow`) toolbars.
    - Allocates remote buffers and reads process memory to extract notification `TBBUTTON` parameters and tooltips directly from the explorer shell process.
    - Exposes `g_TrayTooltipsMap` and `GetScrapedTrayTooltip()` to fetch tooltip texts.
    - Re-populates and updates our taskbar system tray toolbar (`UpdateTrayToolbar`) with the scraped icons.

## Desktop Replacement Component
- **DesktopWindow.h / DesktopWindow.cpp**: Replaces the default Windows desktop shell (`Progman` and `WorkerW` / `SHELLDLL_DefView` / `SysListView32`).
  - **Responsibilities**:
    - Spawns a custom bottom-Z-order desktop window with the native class name `"Progman"`.
    - Spawns a child window class `"SHELLDLL_DefView"`, containing a `"SysListView32"` control representing the desktop icon grid.
    - Hides the native Windows desktop windows (`Progman` and `WorkerW` hosting the native shell view) to prevent duplicate icon lists and overlapping paint events.
    - Intercepts background erase/paint messages to render the desktop wallpaper using GDI+. It caches the decoded wallpaper `Gdiplus::Bitmap` and automatically invalidates/reloads it when the user's wallpaper path, style, or tile configuration changes in the registry.
    - Restores the native desktop windows to visible state upon cleanup to ensure seamless co-existence and system stability.

## Settings & Properties Dialogs
- **EliteSettingsStub.cpp**: Compiled directly into the Control Panel Applet `EliteSettings.cpl` using the `/LD` flag and linked with `settings_resources.res`. Exposes the `CPlApplet` entry point for launching the Settings dialog directly through the Control Panel (`control.exe`) without spawning external processes.
- **TaskbarProperties.h / TaskbarProperties.cpp**: Implements the **Custom Settings** tabbed property sheet.
  - **Responsibilities**:
    - Replicates the native Windows properties look using `CreatePropertySheetPageW` and `PropertySheetW` (the tabbed applet UI).
    - Contains dialog procedures (`TaskbarSettingsDlgProc`, `StartMenuSettingsDlgProc`, `ToolbarsSettingsDlgProc`, `GenericPageDlgProc`).
    - Reads/Writes native standard settings (like Taskbar Lock, Auto-hide) to native Windows registry paths, functioning as a 1:1 true functional replacement for `desk.cpl`/`shell32` dialogs.
    - Reads/Writes our custom extension settings (like Taskbar Mode) to `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`.
    - Uses `PSN_APPLY` to broadcast changes to the shell so `TaskbarWindow` and native Windows respond.
- **resource.h**: The standard header that maps UI element IDs (like `IDC_MODE_REPLACE`, `IDD_TASKBAR_PROPS`, `IDM_TASKBAR_SETTINGS`, `IDI_PREFERENCES`) to integer constants.
- **resources.rc**: The Resource Compiler script. It defines the layout, coordinates, and text for all the Property Sheet Dialog tabs (`IDD_TASKBAR_PROPS`, `IDD_STARTMENU_PROPS`, `IDD_DESKTOP_PROPS`, `IDD_TOOLBARS_PROPS`, plus secret tabs) and embeds Application Icons (`IDI_MAIN_PROGRAM`, `IDI_PREFERENCES`) and the Start Orb (`IDB_START_ORB`).
- **stub_resources.rc**: Embedded resources specifically for the `EliteSettings.exe` stub to ensure it has the correct `PREFERENCES.ico` icon in Explorer.
- **app.manifest**: XML manifest embedded into the final EXE. Requests `Microsoft.Windows.Common-Controls` version 6.0.0.0 to ensure buttons, tabs, and property sheets render with the modern visual style instead of the Windows 95 classic theme. Crucially configures DPI awareness (`PerMonitorV2`).

## Debugging & Utilities
- **Logger.h / Logger.cpp**: Provides the `Log()` and `LogW()` functions. It strictly appends logs to `C:\EliteSoftware\Logs\EliteTaskbar.log`. It is used to trace crashes, verify window states, and log bootstrapper operations.

## Architecture Workflow
1. User launches `EliteTaskbar.exe`.
2. `main.cpp` starts, uses `CommandLineToArgvW` to parse arguments. If `/settings` is passed, it launches the settings property sheet directly and exits.
3. Otherwise, it initializes GDI+ and checks registry for `TaskbarMode`.
4. `TaskbarWindow::Initialize` runs. If `Replace` mode is active, it actively hooks a timer to push `Shell_TrayWnd` (the native taskbar) off-screen (`-10000, -10000`) and issues `SW_HIDE`.
5. `TaskbarWindow` creates itself as an `HWND_TOPMOST` layered window at the bottom of the primary monitor.
6. Child components (`StartButton`, `ClockWidget`) initialize themselves inside the taskbar.
7. The user right-clicks -> "Properties" calls the *native* `shell32.dll` properties dialog or uses our 1:1 custom replacement based on config.
8. The user right-clicks -> "Elite Taskbar Settings" calls `TaskbarProperties.cpp`, generating the custom Property Sheet. Changing mode restarts the shell instantly.
