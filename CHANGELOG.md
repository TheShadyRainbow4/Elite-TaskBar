# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]
### [1.2.0.0] - 2026-07-03
### Changed
- **Taskbar Label Monitor Sync (Ghosting Fix)**: Replaced `MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL)` with a custom `GetActualWindowMonitor(HWND)` helper. This resolves an issue where taskbar labels for minimized windows, or windows moving between monitors, would ghost and stay on the original monitor's taskbar. The helper explicitly checks the window's restored `WINDOWPLACEMENT` to ensure precise monitor tracking regardless of minimized state.
- **Taskbar Button Stability**: Refactored `g_TaskButtons` from `std::vector` to `std::list`. This guarantees memory stability for window titles, completely eliminating a bug where taskbar labels would suddenly disappear or corrupt due to pointer invalidation during vector reallocation.
- **Live Orb Settings Refresh**: The Elite Taskbar Settings UI now instantly applies changes made to the "Start Orb" combobox. It triggers a `WM_SETTINGCHANGE` broadcast with `EliteTaskbarSettings` instantly forcing all taskbar instances to hot-reload `ReloadOrbImage()` without requiring an explicitly clicked "Apply" button.
- **Elite Everything Toolbar Toggle**: Added a registry-backed toggle feature (`ShowEverythingToolbar`). Integrated a context menu item (`WM_COMMAND` 3024) inside the "Toolbars" sub-menu to cleanly hide/show the embedded `EverythingToolbar` using ReBar band manipulation (`RB_SHOWBAND`).
- **Build Chain Integration**: Updated `build_sign.ps1` to digitally sign ALL compiled executable variants including both the root directory and inner `BuildOutput/` directories, completely automating the signing of x64 and x86 files.
- **Settings Tabs Implementation**: Populated custom Property Sheet tabs (Taskbar, Start Menu, Toolbars) with functional UI controls corresponding to the requested features (OpenShell vs Native start menu, Notification Area view modes, multi-monitor component configurations, and toolbars list). 
- **Dynamic Start Menu Triggers**: The Start Orb logic (`StartButton.cpp`) now actively reads `StartMenuMode` from the registry upon click. It programmatically triggers OpenShell via keyboard simulation, native Start Menu via `SC_TASKLIST`, or supports combination triggers (e.g. Shift+Click) based on user preference.
- **Legacy Clock Flyout Customization**: Integrated the `TrayMode` setting into the Clock Widget. Native Mode instantiates the native Windows `ClockFlyoutWindow` via COM, while Legacy Mode accurately mimics the ReactOS behavior by directly invoking `timedate.cpl`.
- **Settings UI Logic**: Added comprehensive dialog procedures (`TaskbarSettingsDlgProc`, `StartMenuSettingsDlgProc`, etc.) to intercept interactions on the new Property Sheet tabs, saving the configuration straight into `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` and properly notifying the shell.
- **Properties and Settings**: Separated the "Properties" and "Settings" options. "Properties" now correctly launches the native Windows desk.cpl applet for native taskbar settings, while the newly added "Elite Taskbar Settings" menu item opens the custom property sheet to manage our shell's features.
- **Admin Privilege Bypass (Z-Order Fix)**: Implemented an aggressive background timer during 'Replace' mode that continually forces the native taskbar off-screen to `-10000, -10000`. This bypasses UIPI (User Interface Privilege Isolation) issues when running as a Standard User, completely preventing the native Windows clock and Show Desktop buttons from visually piercing through our overlay.
- **Custom Properties Sheet Cleanup**: Removed native checkboxes (Lock Taskbar, Auto-hide, Small Icons) from the custom Elite Taskbar Settings property sheet to prevent redundant and unsynced data, dedicating it strictly to Elite Taskbar custom mode toggles.
- **Native Context Menu IDs Fixed**: Corrected the internal Windows Message identifiers (`WM_COMMAND`) for the Taskbar Context Menu to strictly match the native Windows 7/10 Taskbar constants. "Cascade" now uses `403`, "Stacked" uses `404`, "Side-by-side" uses `405`, "Task Manager" uses `420`, and "Lock Taskbar" uses `424`. This fixes the issue where clicking these items previously triggered completely wrong actions (like tiling windows or opening the Date & Time applet).
- **Compile Error Resolution**: Fixed a duplicate `WM_CREATE` switch case that caused the C2196 compiler error, ensuring the aggressive background taskbar hiding timer successfully compiles and initiates.
- **Build System**: Updated `build.ps1` to detect and dynamically copy `EliteTaskbar.ico` from the project root into the `Resources` folder before compilation. This ensures the executable always bakes in the most recent custom icon without manual intervention, while still producing the final binary in both the root and `BuildOutput` folders.
- **Context Menus**: Rewrote taskbar context menu routing to elegantly handle both overlay mode and standalone PE mode. If the native `Shell_TrayWnd` exists, it delegates native commands via `PostMessageW`. If it doesn't exist, it falls back to a standalone implementation.
- **Standalone Mode (PE Environment)**: Added robust fallbacks for when `explorer.exe` is dead/missing:
  - Added native `EnumWindows` based `ToggleDesktop` function to support "Show Desktop" natively.
  - Re-implemented `CascadeWindows` and `TileWindows` directly using the user32 API for manual cascading and tiling.
  - Implemented a custom Native Windows Property Sheet using `PropertySheetW` and `PROPSHEETHEADER` to provide our own "Taskbar Properties" dialog when requested, setting the groundwork for managing custom features while remaining faithful to the tabbed applet look.
  - Linked `comctl32.lib` into the build process for standard common controls.
  - **Taskbar Mode Management**: Added "Taskbar Mode" radio buttons to the Taskbar Properties sheet. You can now toggle the shell between "Independent" (running alongside Explorer) and "Replace" (taking over the entire taskbar). It reads/writes to `HKCU\Software\EliteSoftware\Win32Explorer\Advanced\TaskbarMode`.
  - **Dynamic Shell Reboot**: Mode changes broadcast a `WM_SETTINGCHANGE` `TraySettings` message. The `TaskbarWindow` intercepts this, reloads the registry, and dynamically reboots itself via `IDM_RESTART_SHELL` to fully recreate the overlay shell in the new mode.
- **Context Menu 'Run...' Option**: Added a dedicated "Run..." button to the taskbar context menu using ID `3009`. It perfectly matches native behavior by delegating to the native `ID_SHELL_CMD_RUN` (401) or by falling back to dynamically invoking the undocumented `RunFileDlg` from `shell32.dll` ordinal 61 in standalone mode. Added a WH_CBT hook to forcefully center this dialog on the screen during invocation to prevent weird spawn coordinates.
- **Properties Override**: Fixed the context menu so that the "Properties" button unconditionally opens our newly implemented custom Property Sheet tabbed applet instead of incorrectly triggering the modern Windows "Run" dialog.
- **Dynamic Property Sheets**: Reconstructed the Custom Taskbar Properties dialog to strictly adhere to the `CreatePropertySheetPageW` and `PropertySheetW` API flow requested in the Master Ledger. It now dynamically injects tabs for Taskbar, Start Menu, Desktop, and Toolbars.
- **Secret Debug Tabs**: Implemented registry checks (`HKCU\Software\EliteSoftware\Win32Explorer\Advanced`) and command-line switches (`/devmode`) to dynamically authenticate and inject hidden "Everything Indexer" and "DLL Scanner" property tabs when authorized.

## [1.1.0.0] - 2026-07-02
### Added
- Initial project structure created.
- Added `README.md` and `CHANGELOG.md`.
- Specified `Resources` directory and asset paths in documentation.
- Created root `build.ps1` script for compiling the project.
- Added `backup.ps1` for local project snapshots.
- Added `install_prereqs.ps1` to configure Visual Studio Build Tools and .NET dependencies.
- Refactored `StartButton` into a dedicated `WS_EX_LAYERED | WS_EX_TOPMOST` floating window to allow the Start Orb to cleanly overhang outside the taskbar bounds into the desktop, closely mimicking native Windows 7 behavior.
- Integrated `VK_LWIN` injection for `StartButton` click events to natively hook OpenShell and other third-party start menu replacements.
- Set up active `WM_TIMER` z-order enforcement to ensure the custom Start Orb remains aggressively layered in front of the native OpenShell UI.
- Rewrote GDI+ memory rendering for the `StartButton` animation using a true 32-bpp `DIBSection` to fix alpha-tearing and glitching during hover/press states.
- Fixed DWM Glass rendering on the System Tray and Clock by removing `TextRenderingHintClearTypeGridFit` (which destroyed the alpha channel) and applying `BLACK_BRUSH` rendering to child windows.
- Dynamically scale the Taskbar height to automatically match the user's legacy "Small Taskbar Buttons" / "Large Taskbar Buttons" settings by querying the native `Shell_TrayWnd` `RECT`.
- Implemented Dual-Mode Execution Bootstrapper logic (Independent vs Explorer Replacement).
- Wrote robust `Cleanup()` routines to automatically restart or un-hide the native `explorer.exe` shell when EliteTaskbar exits.
- Implemented `Win32Clock` COM instantiation logic to natively summon the legacy system Clock/Calendar flyout, fully supporting `CLSCTX_ALL`.
- Added the System Tray contextual Right-Click menu, completely mapped to Native Taskbar actions (Lock the taskbar, Properties, Cascade windows, Show the desktop, Task Manager).
- Refined the Taskbar layout engine: registered the `TrayShowDesktopButtonWClass` to reserve the exact 15px right-edge anchor gap.
- Greatly improved native Clock typography: increased the drawing bounding box, dropped `DTT_GLOWSIZE` to fix glass-clipping artifacts, and applied `DT_RIGHT` alignment with native margins.
- Hardened the `TrackPopupMenuEx` implementation using `TPM_RETURNCMD` to guarantee 100% reliable execution of contextual tools without message-pump focus drops.
- Enforced single-instance architecture using a Global Named Mutex, with an undocumented `-allowMultiple` bypass flag.
- Integrated `git push` into the automated build and backup script.
- Added `Backups/` to `.gitignore` to prevent recursive CAB bloat on the remote repository.
- Fixed catastrophic infinite click loop in `ShowLegacyClockExperience` by targeting the correctly preserved `g_hNativeTaskbar` instead of globally matching `Shell_TrayWnd`.
- Added missing `BufferedPaintInit()` and `BufferedPaintUnInit()` to `main.cpp` to ensure the DWM Glass Buffered Paint API (`BeginBufferedPaint`) operates correctly for standard users, fixing missing clock text.
- Expanded the system tray width (`g_hTrayNotify`) and clock widget width (`g_hTrayClock`) while significantly increasing right-padding to mathematically prevent the clock from ever physically clipping underneath the native or custom `TrayShowDesktopButtonWClass`.
- Built an absolute GDI rendering fallback into `TrayClockProc` so the clock remains fully visible if `BeginBufferedPaint` fails entirely when running in restricted environments or via Windhawk injection.
- Fully implemented missing Context Menu commands (`IDM_TASKBAR_CASCADE`, `IDM_TASKBAR_STACKED`, `IDM_TASKBAR_SIDEBYSIDE`, `IDM_TASKBAR_SHOWDESKTOP`) utilizing standard Windows APIs.
- Updated Context Menu `ShellExecuteW` calls to default verbs to ensure UAC elevations function correctly for standard users attempting to launch `taskmgr.exe` or `rundll32.exe`.
- Created comprehensive `Documentation/SourceMap_And_Architecture.md` providing a detailed breakdown of all source files, their purposes, and the overall lifecycle of the taskbar shell extension.
- Removed EverythingSDK and search toolbar from the project.
- Added binary signing to all executables in the build scripts.
- Removed all third-party SDK sources from git tracking (git rm -r --cached).
- Fixed multi-monitor window label assignment by passing valid title pointers via 	bb.iString.
- Implemented GetWindowMonitor that properly detects the cNormalPosition of minimized windows to prevent them jumping taskbars.
- Rewrote StartButton logic to support 3-frame and 4-frame dynamic height ORB images.
- Improved TaskbarProperties UI to preview Orbs via GDI+ and properly isolate Per-Monitor configurations in the Registry.
- Modified StartButton native menu triggers to use SendMessageW and lCursorParam alongside simulated VK_SHIFT injections to handle interactions with Open-Shell accurately.
- Added fixes for taskbar buttons disappearing when moving across monitors.
- **Build System File Locks**: Added aggressive `Stop-Process` check at the absolute beginning of `build.ps1` to automatically terminate running instances of `EliteTaskbar` before compilation. This permanently resolves the `fatal error LNK1104` caused by attempting to link over locked executable files.
- **Native Taskbar Transparency Fix**: Rewrote the taskbar hiding logic in `TaskbarWindow::Initialize`. Instead of using `WS_EX_LAYERED` (which causes Windows to silently un-reserve the AppBar space and lets windows maximize behind it), the native taskbar is now completely clipped out of existence using an empty `SetWindowRgn()`. The taskbar remains fully active, fully positioned, and fully opaque to the OS, guaranteeing correct workspace reservation and preventing windows from going under the custom taskbar!
- **Start Menu Trigger Positioning**: Modified `StartButton::OrbWndProc` to directly utilize `SendMessageW` targeting the transparent native taskbar handle underneath it. Since the native taskbar is now perfectly positioned on every monitor, Open-Shell dynamically hooks it and perfectly aligns the Start Menu where you click.
- **Taskbar Properties UI**: Re-routed the Trigger Combobox inside `StartMenuSettingsDlgProc` to actively query `StartMenuMode` from the registry and dynamically set its default selection index on load (`CB_SETCURSEL`), rather than misaligning against non-existent UI radio buttons.
- **Live Orb Updates**: Fixed a bug where applying Orb changes in Taskbar Properties wouldn't reflect immediately. `WM_SETTINGCHANGE` now correctly iterates over `g_Taskbars` to find matching window instances and explicitly reloads the `StartButton` Orb image on the fly!
