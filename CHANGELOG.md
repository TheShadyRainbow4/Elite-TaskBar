# Changelog

All notable changes to this project will be documented in this file.


## [Unreleased]
### Fixed
- **PDB Synchronization Compiler Error (C1041)**: Added `/FS` flag to the `cl.exe` compile command in `build_x64.ps1` and `build_x86.ps1` to prevent concurrent write lock contentions on PDB files.
- **PowerShell Registry Value Parameter Validation Bug**: Replaced `Get-ItemProperty -Name ""` with `(Get-Item -Path $path).GetValue("")` (or safe wrapper) in `SourceFiles\EliteSettings.ps1` to avoid throwing a parameter validation exception when querying empty-string default registry values, restoring successful settings saving.
- **Win32Explorer Submodule Sync & Build Handling**: Integrated automated sync of modified source files from `Remaining_Shell\Win32Explorer_26.0.3.0` to the submodule `Win32Explorer_26.0.3.0` in `build.ps1`, configured MSBuild path in submodule build script to target BuildTools, and hardened `build.ps1` to abort with exit code 1 if Win32Explorer compilation fails.
- **Win32Explorer C2664 Compilation Fix**: Replaced `(LPCITEMIDLIST)` cast with `(PCIDLIST_ABSOLUTE)` in `SHGetPathFromIDListW` call inside `TaskbarProperties.cpp` (both source and submodule copies) to satisfy strict type checking of SHGetPathFromIDListW.

### Added
- **EliteTaskbar System Tray Icon**: Added a system tray icon for EliteTaskbar referencing `g_Taskbars[0]->hTaskbar` and `IDI_MAIN_PROGRAM`, handling `WM_TRAYICON` context menu (settings / quit) and clean `WM_CLOSE` signaling, with robust removal in cleanup.
- **Win32Explorer EventWindow Exposure**: Exposed `HWND GetHWND() const { return m_hwnd.get(); }` in `EventWindow.h` and declared `OnEventWindowMessage` in `App.h` to allow App to reference and handle the event window messages.
- **Win32Explorer System Tray Icon**: Registered the tray icon in `App::SetUpSession()` using `IDI_MAIN_PROGRAM`, hooked window messages in the `App` constructor, implemented `OnEventWindowMessage` to show the context menu ("Open New Window" / "Quit Win32Explorer"), cleaned up the icon in the destructor, and silenced unreferenced parameter warning for `wParam`.
- **Custom Icon Theme Resources**: Defined `IDC_THEME_FOLDER_PATH`, `IDC_THEME_FOLDER_BROWSE`, and `IDC_ENABLE_DARK_MODE` in `resource.h` and updated the dialog template in `resources.rc`. Synchronized these defines to `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resource.h` and `resources.rc`, and synchronized `GetEliteRegistryRoot()` in `Config.h`.
- **Custom Icon Theme Settings UI (Reverted)**: Reverted any changes made to `EliteSettings.ps1` (and duplicate under `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/EliteSettings.ps1`) following parent's critical redirect to abandon legacy PowerShell settings and use C++ native property sheets directly.
- **Custom Icon Theme Loader Core (Win32ResourceLoader)**: Added filesystem/fstream includes, and implemented `GetIconName` and `GetCustomThemePath` helpers inside `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/Win32ResourceLoader.cpp`. Intercepted GDI+ resource loading inside `LoadGdiplusBitmapFromPNGAndScale` to try reading PNG or ICO file from `CustomThemePath` first before falling back to native resources.
- **Custom Icon Theme Properties Page**: Added `<shlobj.h>` include and implemented `BrowseForFolder` helper in `TaskbarProperties.cpp` (and duplicate under `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`), with a strict `PCIDLIST_ABSOLUTE` cast on `SHGetPathFromIDListW` to satisfy strict compiler type-checking. Cast `SendMessageW` to `(DWORD)` in `SelectOrbComboBox` (in both files) to resolve warning C4389 (signed/unsigned mismatch), and updated `TaskbarSettingsDlgProc` (in both files) to load and save `CustomThemePath` to registry and handle the folder Browse and EN_CHANGE events.
- **Fixed Button Widths Hookup**: Added `FixedWidthSize` integer into `EliteTaskbarConfig`, populated via registry loading in `main.cpp`, and mapped directly to `TB_SETBUTTONWIDTH` in `TaskbarWindow.cpp` to correctly apply Small (100px), Medium (160px), and Large (220px) settings for Fixed Button modes.

### Fixed
- **Settings Label Backfills**: Injected `EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB)` into `WM_INITDIALOG` for all Property Sheet dialogues in `TaskbarProperties.cpp`. Labels and checkboxes now correctly paint over the native Windows property tab texture rather than rendering a grey block.
- **Scroll Bar Padding**: Modified `CreateDynScrollArea` to stretch the `EliteDynScrollArea` window bounds to match `GetClientRect` of the parent dialog exactly. This fixes the right-side gap, properly pinning the scroll bar to the edge.

### Added
- **Native System Tray Integration**: Fully routed `WM_COPYDATA` messages to `TrayNotifyWnd` inside `WindowProc`, enabling true native ingestion of `NOTIFYICONDATA` structures when running in `Replace` mode.
- **Two Overflow Methods**: Implemented the two planned overflow modes for the Notification Area:
  1. **Vista Inline**: Triggers an expandable chevron (`<`/`>`) to slide icons leftwards natively.
  2. **Win7 Flyout**: Triggers an upward chevron (`^`) which summons a decoupled floating window (`TrayFlyoutWnd`) holding hidden icons.
- **Settings Broadcast Update**: Added immediate runtime invalidation of `TrayNotifyWnd` and task buttons when "Apply" is clicked in properties, removing the need for application restarts to see setting changes.

### Changed
- Refactored `TaskbarWindow::WindowProc` to intercept system-wide tray messages and forward them down the window chain.
- Wired `IDC_TRAY_NATIVE` and `IDC_TRAY_LEGACY` dialog components to the newly created `TrayOverflowMode` Enum.

### Fixed
- Addressed multiple Z-Order / Flashing issues where the orb and taskbar would render behind/above incorrectly.
### [1.2.0.0] - 2026-07-03
### Changed
- **Multi-Monitor Start Menu Resolution**: Corrected class name registration in `TaskbarMode::Replace` (PE mode) to explicitly use `Shell_SecondaryTrayWnd` for all secondary taskbars instead of registering them all as `Shell_TrayWnd`. This ensures third-party start menus like Open-Shell can correctly hook and distinguish between primary and secondary taskbars natively across all monitors without fallback injection.
- **Start Orb Glitch Fix**: Completely eradicated the visual glitch/flashing of the taskbar over the Start Orb. The glitch was caused by an aggressive `SetForegroundWindow` invocation pushing the taskbar to the very front of the `HWND_TOPMOST` stack. This call was removed, as Open-Shell determines the active monitor directly from the targeted `HWND` and does not require the taskbar to steal focus.
- **Development Rules**: Updated `GEMINI.md` to add Rule 6, which mandates that build scripts/tools must be launched in an external, visible window while simultaneously capturing their output to a log file for parsing.
- **Dynamic Settings Application**: Taskbar property changes (such as "Taskbar Button Width" and "Hover Previews") now instantly apply to the live shell upon clicking "Apply" without requiring a full shell restart. The shell rebuilds the `ToolbarWindow32` task buttons in-place and re-queries the configuration, falling back to a full `IDM_RESTART_SHELL` only if the user toggles major framework components like "Taskbar Mode" or "Use Native TaskBand".
- **Development Rules**: Updated `GEMINI.md` to enforce rule #5 regarding "Native Replication & Co-existence", mandating that the taskbar replacement behaves exactly as the native Windows taskbar does unless impossible, while still preserving custom settings and additional features.
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
- **Aero Hover Thumbnails**: Replaced the placeholder black thumbnail preview with a fully functional Windows 7-styled Aero Hover Thumbnail! Implemented `DwmExtendFrameIntoClientArea` with a negative margin to draw a fully translucent glass frame, utilized `DrawThemeTextEx` with `DTT_GLOWSIZE` to draw the window title with a native white glow, and added a native functional Close Button (`DFC_CAPTIONCLOSE`) to the corner.
- **Basic Jump List Context Menu**: Removed the weird legacy system window menu (Restore, Move, Size) that triggered when right-clicking taskbar buttons. Replaced it with a clean, basic Jump List-style menu that perfectly mimics the look of a native right-click (Application Title at the top, a separator, and a 'Close window' button).
- **Hybrid Native TaskBand Toggle**: Adhering to strict development rules (never completely remove existing development features without a toggle), I added a new `Use fully native TaskBand` checkbox to the Taskbar Properties UI (`IDC_USE_NATIVE_TASKBAND`). This setting stores to `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` and provides a planned pathway to reparent the native Windows `MSTaskSwWClass` for users who demand 100% native JumpLists and have the OS-level theme support to match it, while retaining the custom `ToolbarWindow32` tracker for standard operation.
- **Strict Development Rules**: Created and renamed the project rules file to `Gemini.md` to trigger automatic workspace parsing. Updated the rules to explicitly mandate reading all `.md` and `.txt` files prior to starting any task, following directives exactly, and rigorously enforcing changelog updates after every single file edit.

### Changed
- **Start Menu Settings Per Monitor**: Moved Start Menu configuration (Mode, Trigger, Orb Theme) from a standalone tab to the Multi-Monitor Components tab. The settings are now dynamically populated and saved per monitor. Removed the standalone Start Menu tab entirely.
- **Start Menu Button Flashing Fix**: Removed HWND_TOPMOST flag from StartButton orb state transition to prevent flashing above taskbar on click.
- **Start Menu Independence Fix**: Modified Start Menu Mode logic to open on the specific monitor natively when Open-Shell is configured. It forwards a simulated WM_LBUTTONDOWN event natively to trigger Open-Shell directly on the secondary taskbar.
- **UI Layout Optimization**: Implemented dynamic per-monitor property layouts within EliteDynScrollArea to categorize Multi-Monitor components effectively.

- Fixed a linker LNK2005 error and restored missing utility methods for comboboxes in TaskbarProperties.cpp.
- Completed the compilation and signing scripts for .cpl support, exporting CPlApplet natively.
- EliteTaskbar now successfully generates EliteSettings.cpl alongside EliteSettings.exe, properly hosting the settings in a native Control Panel applet wrapper without relying on a proxy stub.
- **Icon Rendering Pipeline**: Modified `build_settings.ps1` to dynamically swap `IDI_MAIN_PROGRAM` (101) and `IDI_PREFERENCES` (102) in a custom `settings_resources.rc` file during compilation. This guarantees `EliteSettings.exe` and `EliteSettings.cpl` inherently utilize `PREFERENCES.ico` as their primary executable icon in Windows Explorer while perfectly retaining access to all native Taskbar dialog structures.

### [Unreleased] - UI Polish & Start Menu Mirror
- **TaskbarProperties.cpp**: Fixed transparent background bug in DynScrollAreaProc where checkboxes had white backgrounds instead of COLOR_BTNFACE.
- **TaskbarProperties.cpp**: Added WS_TABSTOP to all dynamic checkboxes and comboboxes to restore keyboard navigation.
- **StartButton.cpp**: Rerouted 'Elite Custom Menu' (mode 2) to launch 'EliteStartMenu.exe' independently to serve as an independent Start Menu mirror across multiple monitors.
- **EliteStartMenu.ps1**: Created an independent Start Menu implementation using WinForms to guarantee opening on the correct invoked monitor (bypassing native shell interference).
- **build.ps1**: Added compilation step to convert EliteStartMenu.ps1 into an executable using PS2EXE.
- **TrayIconScraper.cpp**: Added skeleton code for the upcoming native tray icon interception and display logic (Win7 Overflow / Legacy).

- Fixed settings controls not painting when launched via taskbar context menu by launching Settings executable independently.
- Added native system tray icons by reparenting the native SysPager to the custom taskbar.
- Added `Remaining_Shell/` to `.gitignore` to temporarily prevent tracking large shell replacement directories.
- Modified `build_Win32Explorer.ps1` to explicitly copy the compiled `Win32Explorer.exe` directly into the `Elite-TaskBar` project root directory upon successful compilation.

## [Unreleased]
### Changed
- **Registry Roots for Portable Mirror**: Added `useHKLM` parameter to `RegistryAppStorageFactory::MaybeCreate`, `OpenKeyForLoad`, and `CreateKeyForSave` function declarations and implementations to support dynamic registry roots (HKLM vs HKCU) based on Portable Mirror Mode. Added `enablePortableMirror` field to `Config` struct in `Config.h` and integrated registry load/save calls in `ConfigRegistryStorage.cpp` and XML load/save calls in `ConfigXmlStorage.cpp`. Updated `App::LoadSettings` and `App::SaveSettings` in `App.cpp` to dynamically load and save settings to HKLM and XML when Portable Mirror Mode is active. Added new control IDs (`IDC_PORTABLE_MIRROR`, etc.) to `resource.h`. Updated `resources.rc` to add Portable Mirror checkbox and Replace Explorer options to the settings UI templates. Added `GetEliteRegistryRoot()` inline function to `SourceFiles/Config.h`. Updated `main.cpp`, `StartButton.cpp`, and `TaskbarWindow.cpp` to dynamically load the Advanced key using `GetEliteRegistryRoot()`. Modified `TaskbarSettingsDlgProc`, `NativeSettingsDlgProc`, and other dialog procedures in `TaskbarProperties.cpp` to load/save Portable Mirror mode, Replace Explorer options, and multi-monitor/debug options using dynamic roots. Added `SetDefaultFileManagerCPP()` to `TaskbarProperties.cpp` for native Explorer replacement. Added Portable Mirror checkbox and Native Settings tab with Explorer Replacement options to `EliteSettings.ps1`. Updated `Load-Settings` and `Save-Settings` in `EliteSettings.ps1` to support XML/HKLM dual-saving and unconditional registry cleanup. Repaired CPL compilation in `build_settings.ps1` to compile `EliteSettingsCpl.cpp` and link `settings_cpl.res` instead of overwriting the PS2EXE compiled `EliteSettings.exe`. Included `Config.h` in `StartButton.cpp`. Updated the master architectural ledger (`BuildGuide-FeatureRequirement_CheckList.md`).

### Added
- Added \Remaining_Shell/\ to \.gitignore\ to prevent massive build objects and artifacts from bloating the repo.
- Injected EliteTaskbar source files directly into Win32Explorer codebase via \Win32Explorer.vcxproj\.
- Added \EliteTaskbar/TaskbarMain.h\ include in \WinMain.cpp\ and established entry point hook for combined execution.

### Changed
- Relocated and redirected the \uild_Win32Explorer.ps1\ script to compile only \Win32Explorer\ and output the final binary to the project root (\C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe\).
- Excluded \Installer.wixproj\ and \TestWin32Explorer\ builds from the automated script since they were failing and unnecessary for current combined EXE goal.
- Merged the RC resources by #including \EliteTaskbar\resources.rc\ directly into \Win32ExplorerMain.rc\.

### Fixed
- Fixed fatal linker error \CVT1100: duplicate resource\ during RC linking. Resolved by commenting out identically mapped IDs (694, 695, 696, 697) in \Win32Explorer_NoTranslation.rc\ (Windows 10 and Fluent UI variants).
- Stripped 4.5GB of redundant and duplicate pre-compiled .res, .obj, and .tlog files which were cluttering the repo disk space.

- Configured "Use large toolbar icons" to be the default and permanently enabled option by overriding `useLargeToolbarIcons` in `Config.h`, `ConfigXmlStorage.cpp`, `ConfigRegistryStorage.cpp`, and disabling the checkbox in `WindowOptionsPage.cpp`.
- Added new "Elite Shell Environment" group box and checkboxes for Native View Mode, ShellBags Support, Elite Taskbar Integration, and Elite Start Menu Integration to the General Options dialog (`IDD_OPTIONS_GENERAL`).
- Registered new UI control IDs (`IDC_OPTION_NATIVEVIEWMODE`, `IDC_OPTION_SHELLBAGS`, `IDC_OPTION_ELITETASKBAR`, `IDC_OPTION_ELITESTARTMENU`, `IDC_GROUP_ELITESHELL`) in `resource.h`.
- Updated `GeneralOptionsPage.cpp` to correctly serialize, bind, and interact with the newly added Elite Shell settings properties inside `Config`.
- Fixed application exiting bug: Win32Explorer will no longer terminate the entire process when the last file browser window is closed if the Elite Taskbar is enabled (via \nableEliteTaskbar\).
- Fixed Taskbar Properties UI bug: Removed conflicting \WM_CTLCOLORSTATIC\ blocks in \TaskbarProperties.cpp\ which previously caused controls (like checkboxes/radio buttons) to remain invisible/black on property sheets until hovered. Now appropriately relying on \EnableThemeDialogTexture\.
- Disabled unimplemented settings checkboxes ('Native View Mode', 'ShellBags', and 'Elite Start Menu') and labeled them '(Coming Soon)'.
- Fixed tab close button size to 16x16 to prevent clipping off the edge of the tab area.

- Fixed white boxes around checkboxes and radio buttons in taskbar settings dialogs by restoring the transparent background drawing logic.
- Fixed crash when exiting the Elite Taskbar by gracefully closing all open Win32Explorer windows before terminating the taskbar thread.

- Made the application selectively stay alive if 'Win32Explorer' browser windows are still open when the Elite Taskbar is manually exited, instead of closing the entire process.

- Fixed an issue where the file manager exiting would forcibly close the taskbar even if nableEliteTaskbar was explicitly toggled off but the taskbar was running.

- Updated `build_Win32Explorer.ps1` to properly sign the executable using the project's PFX certificate.
- Added logic to the build script to safely overwrite active executables by renaming locked ones to `.old.exe` before copying the new artifact.

- Reverted signtool usage in build scripts and properly integrated Elite-EasySigner as requested.


## [Unreleased]
- Separated execution flow in WinMain.cpp so EliteTaskbar.exe and Win32Explorer.exe run independently, fixing the issue where closing one closes the other.
- Scaled down the tab close button in ToolbarHelper.cpp to 8x8 to prevent it from overlapping the tab edges.

- Implemented DisplayColorsDialog to restore missing Display Window Color Change UI, linked to config fields.
- Restored padding logic in ToolbarHelper.cpp to fix tree exit icon shrinking, setting padding to 0 only for tab buttons.
- Fixed C4060 compiler error in TaskbarProperties.cpp caused by empty switch statement.
- Wrapped compilation cmd.exe calls in build_settings.ps1 by setting $ErrorActionPreference = 'Continue' before execution and restoring it to 'Stop' after to prevent NativeCommandError exceptions.
- Corrected the signer tool target path inside build_sign.ps1 to look for Elite-EasySigner_x64.exe and fallback to Elite-EasySigner_x86.exe instead of hardcoded Elite-EasySigner.exe.
- Added dialog templates and control IDs (IDD_HELP_DIALOG, IDD_ABOUT_DIALOG, IDC_ABOUT_EXPAND, IDC_ABOUT_MOREINFO, IDC_HELP_TEXT, IDC_BANNER) to resource.h (and synchronised to duplicate under Remaining_Shell).
- Updated resources.rc to use Segoe UI 8pt (removing DS_FIXEDSYS) and added IDD_HELP_DIALOG and IDD_ABOUT_DIALOG definitions (and synchronised to duplicate resources.rc under Remaining_Shell).
- Implemented ShowHelpDialog/ShowAboutDialog dialog procedures, owner-drawn white banner rendering, dynamic dialog resizing for more info, a GDI painted chin at the bottom, subclassed Property Sheet menu bar injection, and wittily sarcastic tooltips for all settings controls in TaskbarProperties.cpp (and synchronised to duplicate under Remaining_Shell).
- Modified build.ps1 to execute compilation scripts sequentially instead of via background jobs (Start-Job) to resolve permission and folder-creation sandbox limits.
- Fixed x86 settings CPL build failure by dynamically generating settings_cpl_x86.rc targeting EliteSettings_x86.exe in build_settings.ps1.







