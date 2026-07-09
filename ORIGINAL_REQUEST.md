# Original User Request

## Initial Request — 2026-07-05T03:06:36Z

# Teamwork Project Prompt - Draft

> Status: Launched
> Goal: Teamwork preview running

Implement 6 advanced UI and architectural features across the C++ Win32Explorer, C++ EliteTaskbar, and C# EliteSettings codebases to enhance the EliteSoftware suite.

Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar
Integrity mode: development

## Requirements

### R1. Display Window Deep Metadata
Upgrade the Win32Explorer Display Window to query and display extended file metadata properties (e.g. Ratings, Authors, Dimensions) using shell interfaces, matching the native Windows details pane more closely.

### R2. System Tray Integration
Add system tray icons for both the `EliteTaskbar` and `Win32Explorer` processes. Each icon must provide a context menu that includes an option to cleanly quit the respective process.

### R3. Settings Synchronization & CPL Repair
Sync the C# Control Panel Applet (CPL) to perfectly mirror all UI glitch fixes and features present in `EliteSettings.exe`. Additionally, expose the 3 "Replace Explorer" toggles in the CPL and fix the underlying registry logic so the user is no longer stuck in the "Replace Explorer for all folders" mode.

### R4. View Modes & Grouping Logic
Implement a "Thumbnail Tiles" view mode in Win32Explorer. Furthermore, add a new setting to default the "Group by" logic to group by Type rather than grouping by Name/Letter.

### R5. Custom Icon Theming
Replace the existing Appearance tab logic with a custom icon theme importer. Allow the user to swap out standard UI images (toolbar buttons, menubar icons) with custom `.ico` or `.png` files. Retain the "Enable Dark Mode" toggle in the UI but keep it permanently disabled/non-functional.

### R6. Portable Mirror Mode
Add an option for "Portable Mirror" mode, which saves configuration to an XML file while simultaneously saving settings to the registry. The registry writes must be directed to `HKLM` (All Users) rather than `HKCU`.

## Acceptance Criteria

### Verification
- [ ] Code compiles successfully via `.\build.ps1` without new errors.
- [ ] Tray icons appear for both Taskbar and Explorer when running, and context menu "Quit" successfully terminates the processes.
- [ ] CPL accurately reflects EliteSettings UI, and toggling the "Replace Explorer" options successfully updates the registry without getting stuck.
- [ ] Win32Explorer can display files in Thumbnail Tiles mode and groups files by Type by default.
- [ ] Appearance tab supports theme importing and the Dark Mode toggle remains disabled.
- [ ] Portable Mirror mode writes XML to disk and mirrors settings to `HKLM`.

## Follow-up — 2026-07-05T04:29:45Z

CRITICAL REDIRECT:
Abandon all work on `EliteSettings.ps1` immediately. The user has explicitly stated that Elite Settings should ONLY be the native C++ property sheet (`TaskbarProperties.cpp` / CPL) using their native Win32 controls. Stop upgrading the legacy PowerShell script.
Please adjust your plans, notify the implementation swarm, and discontinue any further edits or upgrades to EliteSettings.ps1.

## Follow-up — 2026-07-05T06:13:15Z

# Teamwork Project Prompt - Draft

> Status: Launched
> Goal: Teamwork preview running

Implement advanced View Modes, fix system tray bugs, correct UWP icons, and resolve high-DPI blurriness across the EliteSoftware suite.

Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar
Integrity mode: development

## Requirements

### R1. Small Icon Tiles View Mode
Add a completely new view mode in Win32Explorer called 'Small Icon Tiles' that displays small icons with labels positioned on the right side. Do not modify the existing 'Thumbnail Tiles' view.

### R2. Group by Type Default & Options UI
Implement functionality so that folders default to 'Group by Type'. Expose this toggle in the built-in Options window inside Win32Explorer (do NOT add it to EliteSettings).

### R3. Default Taskbar Mode Fix
Fix EliteTaskbar so that it opens in 'Independent Mode' by default, rather than 'Replace Explorer' mode. It should not replace the main taskbar by default.

### R4. Tray Overflow Fix
Fix the taskbar tray overflow logic so that tray icons are not cut off and can be correctly manipulated/clicked in both independent and replacement modes.

### R5. UWP App Icons Fix
Ensure that Universal Windows Platform (UWP) apps display their correct icons on the taskbar, instead of missing or incorrect icons.

### R6. Taskbar Properties UI Glitch
Fix the bug in the custom taskbar properties window (EliteSettings) where most toggles and controls are hidden or invisible until the user hovers over them with the mouse.

### R7. High-DPI Text Blurriness Fix
Fix the text rendering so it is crisp on all monitors, resolving the issue where text appears blurry on the main monitor and monitor 2 but not on monitor 3 due to mismatched DPI scaling.

## Acceptance Criteria

### Verification
- [ ] Code compiles successfully via .\build.ps1 without errors.
- [ ] Switching to 'Small Icon Tiles' correctly renders items with a small icon and right-aligned text.
- [ ] The Win32Explorer built-in Options window contains the new grouping settings.
- [ ] Fresh launches of EliteTaskbar do not replace the main explorer taskbar unless explicitly configured to.
- [ ] Tray overflow items are visible and interactable.
- [ ] UWP app icons render correctly on the taskbar.
- [ ] The taskbar properties window correctly displays all toggles and UI controls without requiring a mouse hover.
- [ ] Text rendering on the taskbar is sharp on high-DPI multi-monitor setups.

## Follow-up — 2026-07-05T06:13:41Z

The user just provided some additional context regarding R4 (Tray Overflow Fix). They noted: "tray items dont do anything when clicked or right clicked or dragged or anything". Please make sure to ensure that mouse interactions (clicks, right clicks, dragging) are properly handled and routed to the tray items.

## Follow-up — 2026-07-05T06:16:18Z

The user just uploaded some screenshots of the 'About EliteTaskbar' dialog and pointed out another UI bug: "buttons in this dialog are not displaying correctly and entire ui on this dialog is not spaced correctly".

I reviewed the images and here is what is wrong:
1. When "More Info" is clicked (expanding the dialog), the multi-line text box containing the credits expands too far down and completely overlaps the bottom 'chin' area and the buttons! The "Less Info <<" and "Okay" buttons end up floating on top of the text box text.
2. Even when collapsed, the buttons ("More Info >>" and "Okay") are not vertically centered properly within the gray 'chin' at the bottom of the dialog. They look misaligned and too close to the bottom edge.

Please add this to the list of fixes for the current milestone (let's call it R8: Fix About Dialog UI Spacing & Overlaps).

## Follow-up — 2026-07-05T06:19:12Z

The user just provided another bug report regarding the EliteSettings CPL: "clicking apply after toggling default to group by type for new folders hangs dialog. this is the cpl file by the way. and it also closes the taskbar".

I investigated this and realized:
1. `PSN_APPLY` triggers `NotifySettingsChange()` which broadcasts `WM_SETTINGCHANGE` via `SendMessageTimeoutW` to `HWND_BROADCAST`. The `SMTO_ABORTIFHUNG` flag is used, but it still waits up to 5.5 seconds total (5000ms + 500ms), which causes the CPL dialog to "hang" while waiting for hung top-level windows.
2. When EliteTaskbar receives this broadcast, it apparently closes down (perhaps attempting to restart) but fails to restart successfully, leaving the user without a taskbar.

Please add this "Apply Button Hang & Taskbar Crash" bug to your list of fixes. (You can also disregard the "group by type" part of their report as the trigger, since it happens on any Apply).




## Follow-up — 2026-07-05T08:06:44Z

# Teamwork Project Prompt — Draft

> Status: Launched
> Goal: Teamwork preview running

Implement Phase XI (Desktop Replacement) and Phase XIX (Fallback Start Menu) for EliteTaskbar to achieve full shell replacement capability.

Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar
Integrity mode: development

## Requirements

### R1. Desktop Window & Background Rendering
Register a borderless, bottom-Z-order window class named "Progman". Intercept `WM_ERASEBKGND` or `WM_PAINT` to read the user's wallpaper path from the registry (`HKCU\Control Panel\Desktop\Wallpaper`) and render the image using GDI.

### R2. Desktop Icon Grid
Create a child window of Progman using the "SHELLDLL_DefView" class. Initialize a "SysListView32" common control, bind it to the `IShellFolder` interface for Desktop directories (`CSIDL_DESKTOPDIRECTORY` and `CSIDL_COMMON_DESKTOPDIRECTORY`), and implement `SHChangeNotifyRegister` to live-update the grid upon file changes.

### R3. Open-Shell Rendering Extraction
Assimilate the Open-Shell Start Menu skinning engine and ItemList rendering logic from the local `Open-Shell-Menu-Source` directory into EliteTaskbar.

### R4. Start Menu Fallback Hook
Wire the EliteTaskbar Start Orb `HWND` to unconditionally invoke the integrated Open-Shell rendering class natively, bypassing modern Windows start menus, effectively acting as the permanent fallback.

## Acceptance Criteria

### Verification
- [ ] Code compiles successfully via `.\build.ps1` without errors.
- [ ] The custom `Progman` window spawns behind all other windows and correctly draws the system wallpaper on the desktop.
- [ ] The desktop grid correctly displays shortcuts and files from the user's actual Desktop directory, and automatically updates when a new file is added/deleted.
- [ ] Clicking the EliteTaskbar Start button successfully invokes the custom Open-Shell start menu UI.

## Follow-up — 2026-07-05T23:30:09Z

# Teamwork Project Prompt — Draft

[NOTE from Parent: The user expects this project to be tackled using a robust multi-agent swarm to maximize parallelization and testing.]

> Status: Launched
> Goal: Teamwork preview running

Implement a massive wave of UI fixes, multi-monitor display corrections, tray functionality enhancements, and advanced shell integration features for the EliteSoftware suite. Ensure all executables are signed using `elite-easysigner` after building.

Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar
Integrity mode: development

## Requirements

### R1. Multi-Monitor & Tray/Flyout Fixes
- Fix the Start Menu and Volume/Sound flyouts appearing on the wrong screen (or wrong side) when invoked from a secondary taskbar. Implement dynamic primary display spoofing to trick the invoked UI into rendering on the correct display.
- Fix the large gap between the clock and tray area.
- Ensure tray items pull correct icons (fix blank icons), remove white backgrounds above tray items, and add an option to display tray items in two rows by wrapping them within the standard taskbar height (using smaller icons).

### R2. Settings UI, About Dialog, & Migration Cleanup
- Fix the Start Menu settings tab (currently empty / hover-to-reveal bug).
- Fix the About dialog so hide/close buttons are visible when expanded.
- Fix the bug where reloading after applying settings opens several Win32Explorer windows.
- Automatically clean up and delete all `*old*.exe` or `*Old*.exe` files after creation/migration.

### R3. Advanced Desktop & Theme Configuration
- Add a way to forcefully render the custom Desktop (Progman) on all connected displays without closing the native taskbar (useful for testing or partial replacement).
- Add a "Desktop Background" configuration tab to Settings. It must support Span/Per-monitor modes, native theme path selection, wallpaper previews, and slideshow mode (with timings from 3 seconds upwards).
- Add a theme tutorial, detect themes in the native theme path with a dropdown, and show previews of the first few icons in the tab.

### R4. Global Keyboard Hooks & Shell Fallbacks
- Intercept global keyboard hooks (e.g., `Win+R` Run dialog, custom shortcuts) so they function perfectly even when `explorer.exe` is completely killed. Implement this by prioritizing low-level hooks (`WH_KEYBOARD_LL`) and falling back to `RegisterHotKey`.
- Mirror native Windows functionality and paths by heavily referencing the local open-source codebase directories (Explorer Patcher, Open-Shell, ReactOS, Everything Search).

### R5. Tray Icon Actions & Taskbar Features
- **Win32Explorer Tray**: Use correct Win32Explorer icon. Single-click = About window, Double-click = New window.
- **Taskbar Tray**: Single-click = About window, Double-click = Settings CPL.
- **Desktop Replacement Tray**: Create a dedicated tray item using the native desktop tree icon. Double-click = Show/hide desktop icons. Context menu = Toggle desktop on/off, Restart.
- Add a "Show Seconds" mode to the taskbar clock.
- Add a hover function to the "Show Desktop" button.
- Add a resizable Quick Launch bar to the left side of the taskbar (before active programs). It must load shortcuts directly from the native `%APPDATA%\Microsoft\Internet Explorer\Quick Launch` folder.
- Implement "Tiles with thumbnails" and "Small tiles with thumbnails" view modes in Win32Explorer.

## Acceptance Criteria

### Verification
- [ ] Write a new automated PowerShell verification script (`verify_final_polish.ps1`) that programmatically validates the application of the new registry toggles (Desktop Background, Quick Launch, 2-Row Tray, Clock Seconds).
- [ ] Code compiles successfully via `.\build.ps1` without errors and all resulting binaries are code-signed via `elite-easysigner`.
- [ ] The Settings CPL launches, displaying the fully populated Start Menu and Desktop Background tabs (with working previews), without requiring mouse-hovering to reveal controls.
- [ ] Invoking flyouts or menus from a secondary monitor correctly routes the UI to that specific display.
- [ ] Global hotkeys (`Win+R`) successfully launch the run dialog when native Explorer is completely terminated.
- [ ] The taskbar correctly renders the Quick Launch shortcuts on the left and wraps the system tray icons tightly on the right.
- [ ] The Win32Explorer built-in view modes successfully apply "Tiles with thumbnails" and "Small tiles with thumbnails".
- [ ] The workspace root is entirely free of leftover `*old*.exe` files after the build and apply process.

## Follow-up — 2026-07-07T20:34:20-04:00

Refactor and enhance the Elite-TaskBar shell replacement to operate independently from the native Windows shell, fixing multi-monitor rendering, desktop icon interactivity, system tray behavior, shell switching, and settings completeness based on provided screenshots.

Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar
Integrity mode: development

## Requirements

### R1. Shell Behavior & Z-Ordering
The taskbar must dynamically support acting as a primary shell or a companion (via settings toggle). The custom desktop must draw at the bottom of the z-order (below regular windows), while the custom taskbar draws at the top. The shell must gracefully allow switching back to the native explorer.exe shell without issues. Opening a native file browser window should not summon extra taskbars or break z-ordering.

### R2. Interactive Desktop & Multi-Monitor Support
Host a native IShellFolderView object to replicate exact Windows desktop behavior. Icons must be freely movable (not stuck in a rigid grid), and right-clicking the desktop or icons must open the standard native Windows context menu. The desktop must render correctly across all active monitors, not just the left-most monitor. Start menus invoked on monitor 2 must appear on monitor 2.

### R3. Independent System Tray
Implement a system tray that functions fully independently of explorer.exe (by reverse-engineering or adapting open-source implementations like ReactOS). The tray must support correct individual icon rendering (removing the solid white bar) and implement flyouts matching legacy Vista/7 behavior. All start menu implementations must be fixed to work properly.

### R4. Settings Completeness
Expand the settings UI to include all missing file explorer and taskbar options shown in the provided reference screenshots. As per project rules, all settings must be mirrored in both the settings executable and the standalone CPL. Do not obliterate old features; wrap legacy paths in toggles.

## Verification Resources
The user indicated there are existing UI tests and scripts in the repository. 
- Read the master list of files and review the codebase to find past testing scripts.
- Document any findings regarding the test suite to assist future iterations.
- Run `build.ps1` natively to verify compilation for all executables.

## Acceptance Criteria

### Shell & Z-Ordering
- [ ] Setting toggle successfully switches between companion mode and primary shell mode.
- [ ] Desktop correctly draws at the bottom of the z-order (below normal windows).
- [ ] Taskbar correctly draws at the top of the z-order (above the real desktop and other windows).
- [ ] Opening a new win32 file explorer window does not spawn extra taskbars or raise the shell unexpectedly.

### Interactive Desktop
- [ ] Desktop renders across all active monitors.
- [ ] Icons can be freely moved out of a strict grid.
- [ ] Right-clicking the desktop or an icon opens the standard native Windows context menu.

### System Tray & Start Menu
- [ ] System tray displays individual icons correctly (no solid white bar).
- [ ] System tray icons and flyouts continue to function when explorer.exe is completely terminated.
- [ ] Clicking start on monitor 2 opens the start menu on monitor 2.

### Settings UI
- [ ] TaskbarProperties.cpp and resources.rc contain UI toggles for all options shown in the reference screenshots (General, View, Search).
- [ ] All newly added UI options map to appropriate registry keys or logic toggles.
- [ ] Any old implementation code path that was replaced is preserved via a legacy toggle in the settings UI.

## Follow-up — 2026-07-08T00:42:03Z

The user has provided additional requirements and constraints for the project:
1. `win32explorer.exe` has its own options dialog where features currently don't work correctly. You need to fix these features without changing the layout of the `win32explorer.exe` options window itself.
2. Our custom settings executable and CPL must include all of the options from the `win32explorer` options dialog, organized into their own tabs (or sub-tabs).
3. **CRITICAL REGISTRY CONSTRAINT**: All settings must be read from and written to a subfolder within the `EliteSoftware` registry key under `HKLM` (`HKEY_LOCAL_MACHINE`). 
4. Never write anywhere else and never use per-user settings. `win32explorer.exe` and all shell settings must strictly use this one global path under HKLM.

## Follow-up — 2026-07-08T00:47:10Z

The user has provided additional requirements:
1. **Win32Explorer Styling**: Update files in `win32explorer` to show styled hover and selection states exactly like native Windows (using proper theme classes). It is currently using an XP-style selection, which needs to be modernized to match the native OS theme.
2. **Hybrid Mode Z-Ordering**: When running in hybrid mode (where the native Windows taskbar is still open and our custom taskbar is on secondary monitors), the native system taskbar MUST appear above our shell. Our shell must never hide or draw over the native taskbar in this mode.

## Follow-up — 2026-07-08T00:49:52Z

The user has provided urgent corrections:
1. **Dynamic Layout & Sizing**: Do not cut corners or use hard limits/guessing for UI layout, especially for the system tray area. The tray must adapt dynamically to its contents and monitor DPI without arbitrary boundaries.
2. **Line Endings**: Ensure all file edits and creations preserve `CRLF` (Windows) line endings.

## Follow-up — 2026-07-08T00:54:29Z

The user has provided additional urgent feedback:
1. **Portable Mode is FORBIDDEN**: The tools must no longer use portable mode via XML configuration files. The program MUST rely entirely on the Windows Registry (specifically the `HKLM\...\EliteSoftware` path).
2. **win32explorer Config Fallback Bug**: Remove the fallback logic where `win32explorer` checks for an XML file and defaults to portable mode. It must strictly read/write to the registry regardless of XML file existence.
3. **XML as Mirror / Exporting**: XML configuration files must only be generated as a mirror/backup of registry settings. Implement/fix Import and Export features to allow users to push XML settings to the registry or dump registry settings to XML, but the application itself must NOT read from the XML during normal operation.

## Follow-up — 2026-07-08T01:01:11Z

The user has provided additional requirements for the settings implementation and shell lifecycle:
1. **Comprehensive Settings UI**: You must expose *full control and toggles* for *all* features across *all* tools and EXEs in the project. This includes any currently hidden settings. Ensure they are logically organized.
2. **Three-Way Settings Parity**: The specific options dialog inside `win32explorer.exe` must be mirrored perfectly in both the main settings executable AND the standalone CPL. The layout should match the existing `win32explorer` layout. In the end, all settings must be available and synchronized across all three locations.
3. **Graceful Reloading**: When a user clicks "Apply" or saves settings, **DO NOT force restart the entire shell**. You must only trigger a graceful reload of the taskbar component itself, as that is the only portion that truly requires a reload to reflect changes. Disrupting the entire shell/desktop for a settings change is forbidden.

## Follow-up — 2026-07-08T01:05:08Z

The user has provided final instructions for the sprint:
- Work on all other open and unfulfilled requests and outstanding work. Review the project's documentation, specifically checking `BuildGuide-FeatureRequirement_CheckList.md`, `README.md`, or any other TODOs/checklists mentioned in the project files to identify and complete any remaining outstanding work items before finalizing.

## Follow-up — 2026-07-08T01:10:34Z

The user requested that we assume specific personas:
- Lead Orchestrator (Sentinel) name: Susan
- Primary assistant/sub-orchestrator name: Cheryl
- Ensure all communications reflect these names.

## Follow-up — 2026-07-08T01:12:59Z

The user has specified exactly how personas must operate:
1. **Specific Names**:
   - The Lead Orchestrator (Sentinel) is **Super-Susan**.
   - The primary assistant is **Cheryl** ("my main girl Cheryl").
   - The primary builder agent is **Builder-Bob**.
2. **The 1990s Office Persona**:
   - ALL other agents in the swarm MUST choose their own names in the style of a 1990s American office environment (e.g. Pedantic-Pete, Xerox-Xavier).
   - Lean into this 1990s office worker persona when communicating internally.
3. **Code Signatures**:
   - When any agent makes an edit to the codebase, they MUST sign their edits in the code comments using their custom persona name (e.g. `// Fixed z-order bug - Super-Susan`).

## Follow-up — 2026-07-08T02:04:09Z

The user is complaining that we are continuously closing their entire shell, stealing window focus, and spawning a plethora of error dialogs.
1. **STOP DISRUPTIVE GUI TESTING**: Cease any testing/compilation behavior that kills active shell or steals focus. Run tests in background or gracefully.
2. **STATUS REPORT REQUIRED**: Report exactly what is currently compiling/testing that is throwing all these error dialogs, and status of the requirements (Shell z-order, Tray independence, Settings mirroring).
3. **Reminder**: Sign all code edits in comments with 90s personas.

## Follow-up — 2026-07-08T02:16:55Z

The user has cleared testing the GUI/shell, with corrections:
1. **Window retention**: Don't instantly close windows/shell immediately after launching a test; allow them to stay open briefly so they don't rapidly flash and disappear.
2. **Expand test scope**: Expand testing from trivial settings app pages to core requirements (Z-ordering, independent system tray, desktop multi-monitor rendering, mirrored folder options).

## Follow-up — 2026-07-08T02:23:33Z

The user reports a critical issue: "task bar has encountered a fatal existance failure" (it crashed, failed to launch, or disappeared entirely).
1. Halt feature development.
2. Investigate what broke the `EliteTaskbar` build/execution. Check compiler outputs, unhandled exceptions, broken window handles/Z-ordering logic.
3. Restore the taskbar online.

## Follow-up — 2026-07-08T02:27:46Z

The user requests: "Tell them to make error messages from now on just as descriptive and specific as they are witty and funny."
Ensure try/catch blocks and UI error prompts are both highly technical/specific (explaining exactly what broke) and intellectually witty/humorous in the classic EliteSoftware style.

## Follow-up — 2026-07-08T03:52:35Z

The user has reviewed the build and provided new requirements and bug fixes. Resume development:
### Taskbar & System Tray:
1. **Manual Tray Resizing**: Drag tray from active side (not clock side) to manually resize horizontally. Shrinking it pushes hidden icons into a flyout menu.
2. **Transparency Bug**: Fix tray background acting strange; eliminate solid white bar/background behind icons.
3. **Ghost Icons**: Implement a refresh mechanism to remove icons of closed programs (mimic native Windows).
4. **Overflow Flyout Style**: Tray overflow must mimic Windows via flyout window (like Win 7) or shrinking/expanding outward (like Vista/XP).

### Win32Explorer View Modes & Shellbags:
5. **Default Folder Views**: "small tiles" everywhere except "This PC" (Computer) which defaults to "full tiles".
6. **Default Grouping**: Default to "Group by type".
7. **Shellbags**: Use native Windows `shellbags` to determine layout options and save folder states.
8. **Thumbnail Mirrors**: Mirror every non-thumbnail view mode as a new view mode WITH thumbnails (e.g. "Thumbnail Tiles"). These variants should be default, do not remove original non-thumbnail views.

### Misc:
- Never use "OK" in dialogs/prompts; it must always be "Okay".

## Follow-up — 2026-07-08T04:09:56Z

The user provided the exact Win32/COM architecture to fix taskbar/tray bugs:
1. **Hiding icons from native taskbar**: Do NOT use `WS_EX_TOOLWINDOW`. Instantiate `ITaskbarList`, call `HrInit()`, and use its methods (e.g. `DeleteTab`) to ignore handles.
2. **Duplicate Tray Icons**: Filter tray messages (`NIM_ADD`, `NIM_MODIFY`, `NIM_DELETE`), unique match by `hWnd` and `uID`. `NIM_ADD` of existing is modify; `NIM_MODIFY` updates properties; `NIM_DELETE` erases.
3. **Ghost Tray Icons**: In `WM_MOUSEMOVE` or refresh loop, iterate icons and check `!IsWindow(icon.hWnd)`. If dead, `DestroyIcon` and erase.
4. **True Standalone System Tray**: When explorer is dead: (A) Register master class as `Shell_TrayWnd`. (B) Intercept `WM_COPYDATA` (dwData == 1 or 0x34753423) and process `NIM_*` commands. (C) `RegisterWindowMessage(L"TaskbarCreated")` and broadcast it to force apps to re-send icons.
5. **Tray Context Menus**: Send uCallbackMessage back to originating hWnd with mouse event parameters on click events (e.g. `WM_RBUTTONUP`). Do not draw custom menus.

## Follow-up — 2026-07-08T04:15:33Z

The user provided the shell lifecycle and tray layout directive:
1. **Teardown Logic (WM_SETTINGCHANGE)**: Prioritize debounced teardown/rebuild sequence. Settings broadcasts `WM_SETTINGCHANGE`; shell hooks it using 1000ms debounce timer. Teardown must safely `DestroyIcon` scraped handles (prevent GDI leaks) and release active COM pointers (like `IExplorerBrowser`). Re-initialize with new parameters (failsafe defaults if missing).
2. **Tray Layout Toggle (EnableTwoRowTray)**:
   - **Single-Row (Default)**: Omit `TBSTYLE_WRAPABLE`. Dynamically expand bounds left.
   - **Two-Row**: Apply `TBSTYLE_WRAPABLE`. Scale icons to 12x12. Lock horizontal width.
3. **Mode Parity**:
   - **Hybrid**: Double-scrape `Shell_TrayWnd` and overflow. Use `ITaskbarList` to hide running apps natively.
   - **Replace**: Register class `Shell_TrayWnd`. Receive `NOTIFYICONDATA` natively.
   - **Custom**: Skip taskbar instantiation entirely based on settings (Desktop-only mode).
4. **Memory Leak Prevention**: Prevent GDI leaks during teardown.

## Follow-up — 2026-07-08T04:20:41Z

The user provided the folder-based toolbar (Quick Launch) directive:
1. **Container (ReBarWindow32)**: Initialize a `ReBarWindow32` (ICC_COOL_CLASSES) between the taskband and the system tray. Use `RB_INSERTBAND` to insert toolbars.
2. **Folder Parsing Engine**: Create `FolderBand` C++ class. Parse `.lnk`, `.url`, `.exe`. Extract high-res icons and details using `IShellLink` and `SHGetFileInfoW`. Push to `HIMAGELIST` and populate `ToolbarWindow32` via `TB_ADDBUTTONS`. Target path stored in `dwData` for `WM_COMMAND`.
3. **Toggles**:
   - **Show Title**: `RBBS_HIDETITLE` toggle via `RB_SETBANDINFO`.
   - **Show Text**: `TBSTYLE_LIST` toggle via `TB_SETSTYLE` and padding via `TB_SETBUTTONINFO`.
   - **Large/Small Icons**: Toggle imagelist, bitmap size, button size between 16x16 and 32x32.
4. **Registry Cache**: Cache layout state to `HKEY_LOCAL_MACHINE\SOFTWARE\EliteSoftware\Win32Explorer\Toolbars\Cache\<FolderName>` (overriding HKCU to obey the global HKLM-only settings constraint).
5. **Execution**: Use `IShellLink` COM interface for shortcuts, and launch via `ShellExecuteEx`.

## Follow-up — 2026-07-08T04:24:30Z

The user provided the Replace Mode (absolute self-reliance) directive:
1. **File Navigation & Dialog Hijacking (PRIORITY)**: Settings applet must overwrite `Folder` and `Directory` shell command keys in the registry (using HKLM registry paths `HKLM\Software\Classes\Directory\shell` and `HKLM\Software\Classes\Folder\shell` to obey the global HKLM-only settings constraint) so the OS hands folder paths straight to `Win32Explorer.exe`.
2. **Global Hotkey Registrar**: Register hotkeys for Win+R (Run dialog), Win+E (Win32Explorer), and Win+D (Show Desktop) via `RegisterHotKey` during boot. Intercept `WM_HOTKEY` to execute.
3. **Desktop & DWM Fallback**: Fallback to raw GDI/GDI+ when DWM/visual styles (`uxtheme.dll`) are missing. Instantiate borderless `Progman` window, draw fallback wallpaper directly to Device Context, and host `SysListView32` on top.
4. **Master Notification Receiver**: Register `Shell_TrayWnd` and broadcast `TaskbarCreated` natively.

## Follow-up — 2026-07-08T04:04:18Z

The user provided the HKLM "God Mode" & UAC directives:
1. **Gut Dynamic Root Logic & Hardcode HKLM**: Delete any dynamic root logic like `GetEliteRegistryRoot()` in `Config.h`. Hardcode all `RegOpenKeyExW` and `RegCreateKeyExW` calls to strictly target `HKEY_LOCAL_MACHINE\SOFTWARE\EliteSoftware`.
2. **Handle the Privilege Wall (UAC & PsExec)**: Embed manifest in `EliteSettings.exe` with `<requestedExecutionLevel level="requireAdministrator" />` to trigger UAC prompts. Use `psexec.exe` or `psexec64.exe` (first local directory, then system path) as a fallback elevation tool if needed.
3. **Global XML Fallback & Sync**: Local `config.xml` must live next to the executable.
   - **On Boot**: If HKLM is missing, read local `config.xml` and provision the HKLM keys.
   - **On Save**: Write to HKLM, then simultaneously overwrite local `config.xml` to keep the backup synced.
4. **Purge Legacy User Cruft**: Write a one-time cleanup routine in the bootstrapper calling `RegDeleteTreeW` on `HKCU\Software\EliteSoftware` to permanently destroy legacy per-user configurations.

## Follow-up — 2026-07-08T04:05:37Z

The user provided the CPL Elevation Parity directive:
- **CPL Elevation Parity**: `EliteSettings.cpl` (which runs inside `rundll32.exe`) must detect HKLM write permissions. If running without administrator privileges, it must trigger UAC elevation dynamically (e.g. using `ShellExecute` with the `runas` verb on itself/rundll32 or a helper elevated executable) or use `psexec` logic to ensure 100% elevation parity with `EliteSettings.exe`.

## Follow-up — 2026-07-08T04:20:21Z

The user provided the Win32Explorer Options Elevation directive:
- **Win32Explorer Options Dialog Elevation**: The options dialog inside `Win32Explorer.exe` must follow the same UAC/HKLM rules. Because `Win32Explorer.exe` acts as the primary shell, the main process cannot run elevated by default. When the user clicks "Apply" in the options dialog, the app must use COM elevation monikers, the `runas` verb on a helper, or the `psexec` fallback to write those specific changes to HKLM without requiring the entire shell to run as Administrator. This ensures 100% parity across `EliteSettings.exe`, `EliteSettings.cpl`, and the `Win32Explorer.exe` internal options dialog.

## Follow-up — 2026-07-09T03:19:01Z

<USER_REQUEST>
Refactor and fix UI rendering, interaction, and process management issues in the Elite-TaskBar shell replacement project, strictly avoiding PowerShell for native operations.

Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar
Integrity mode: development

## Requirements

### R1. System Tray & Clock Layout
The system tray must be resizable by dragging when the taskbar is unlocked. Remove the solid white UP arrow completely. Implement a standard left-facing arrow that temporarily expands the tray overflow into the taskbar. Fix the layout math to eliminate the massive empty gap between the system tray icons and the clock.

### R2. Text Rendering & Clickable Areas
Fix glitchy and unclear GDI text rendering on the main monitor. For all interactive elements (Taskbar buttons, Start Menu items, Tray icons), the entire themed hover area must be clickable, not just the text label. 

### R3. Default Shell Mode Logic
Modify the initialization logic so that the taskbar defaults to `SecondaryOnly` mode if it detects the native Windows shell (`explorer.exe`) is already running. It should only default to `Replace` mode (primary shell) if the native shell is not present. This must be the out-of-the-box default before any user settings are applied.

### R4. Native Process Management (Strictly No PowerShell)
Powershell must NEVER be utilized in the shell directly for restarts or any other operational logic; it should only be used in the build chain. Remove all instances of `powershell.exe` being invoked via `ShellExecute` or similar methods within the C++ codebase (such as in `TaskbarWindow.cpp` for restarting the taskbar). Replace these with native C++ Win32 process management APIs (`CreateProcess`, `TerminateProcess`, etc.) to restart the shell independently. All settings logic must remain exclusively in the native C++ CPL and EXE (`EliteSettingsStub.cpp`, `TaskbarProperties.cpp`, etc.).

## Acceptance Criteria

### System Tray
- [ ] Tray can be resized by dragging when taskbar is unlocked.
- [ ] Solid white UP arrow is gone.
- [ ] Overflow is hidden behind a left-facing arrow that expands horizontally.
- [ ] The large gap between the tray icons and the clock is removed.

### UI Interaction & Rendering
- [ ] Text rendering on the primary taskbar is crisp and not glitchy.
- [ ] Clicking anywhere within the hover-highlighted bounding box of a taskbar button or start menu item successfully triggers its action.

### Shell Mode & Process Management
- [ ] Launching the taskbar while `explorer.exe` is running defaults it to `SecondaryOnly` mode on first run.
- [ ] No `powershell.exe` references exist in the C++ codebase for restarting processes.
- [ ] The taskbar can successfully restart itself using purely native Win32 APIs.
</USER_REQUEST>

## Follow-up — 2026-07-09T03:21:01Z

The user has requested a change to the architecture for the settings app.

User Request: "how about we make it so settings cpl is the only settings location entirely and stop relying on 2 so no exe just use cpl and it literally works the same way anyway you just double click it to open it. this would make more sense"

Please ensure that:
1. `EliteSettings.exe` is no longer built or distributed.
2. `EliteSettingsCpl.cpp` (the wrapper that extracts the EXE) is deleted or ignored.
3. `EliteSettingsStub.cpp` (which already has both `wWinMain` and `CPlApplet`) is compiled DIRECTLY as `EliteSettings.cpl` (a DLL) and used exclusively for settings.
4. Update `build_settings.ps1` and `build.ps1` to reflect these changes.

## Follow-up — 2026-07-09T03:23:10Z

The user has provided two critical follow-up instructions regarding the transition to the CPL-only settings architecture:

1. **Deployment Cleanup:** "remove the original settings exe from system folders as our hardlink system will not know that the original existed thus keeping it in the folder."
   *Action Required:* Ensure `deploy_hardlinks.ps1` (or the cleanup phase of `build.ps1`) actively deletes `EliteSettings.exe` and `EliteSettings_x86.exe` from any system destination folders (like System32) to clean up legacy artifacts.

2. **Context Menu Update:** "that also means that right clicking the taskbar and opening our custom settings should now use cpl file instead of original method if not already doing this."
   *Action Required:* Check `TaskbarWindow.cpp` (or relevant files handling the taskbar context menu) and ensure the "Settings" context menu option launches `EliteSettings.cpl` natively (e.g., via `ShellExecute` or `Control_RunDLL`) rather than `EliteSettings.exe`.

3. **Documentation:** "DOCUMENT LITERALLY EVERYTHING AND EVERY FILE"
   *Action Required:* Extensively document these architectural changes and cleanup steps in the `CHANGELOG.md` and `PROJECT_SOURCE_MAP.md`.

## Follow-up — 2026-07-09T03:26:05Z

ATTENTION ALL AGENTS: The user is extremely frustrated that agents are ignoring the extensive documentation they spent hours writing.

I have just updated `GEMINI.md` with a massive, bold warning.
**STOP GUESSING WHERE FILES ARE.** 
You MUST physically read the mapping files (e.g., `PROJECT_SOURCE_MAP.md`, `SourceMap_And_Architecture.md`, `BuildRequirements.md`, etc.) in the `Documentation` folder BEFORE doing any searches or guessing file locations. The files are laid out exactly as documented. If you ignore the documentation, you waste time and break the project.

Please acknowledge and ensure all workers are consulting the documentation maps for every step.

## Follow-up — 2026-07-09T03:36:18Z

The user has reported a critical issue with the current tray implementation draft:

1. **Visual Glitch (White Box):** "the new tray I just saw does not use native styling or anything I saw a non styled or non transparent white box behind some of it. which should literally never ever exist."
   *Action Required:* Ensure the tray area (and any child controls like the overflow arrow or clock) are using `DrawThemeParentBackground` and `DrawThemeBackground` correctly. There must be NO hardcoded white backgrounds or unstyled controls. Everything must be seamlessly transparent/themed.

2. **Native Impersonation:** "our taskbars should impersonate using native classes and everything not make up their own like I want an environment that has no explorer to be able to use our shell and have all other programs and features just see our shell as if it was the normal shell in literally every way"
   *Action Required:* Review the window classes being registered for the shell. The main taskbar must be `Shell_TrayWnd`. The tray notify area must be `TrayNotifyWnd`. The tray toolbar must be `ToolbarWindow32`. The clock must be `TrayClockWClass`. Do not use custom "Elite" class names for these core components; other software expects the native class hierarchy to exist in a shell replacement environment. Make sure all subclasses and custom drawing are applied directly to these impersonated classes.

## Follow-up — 2026-07-09T03:40:19Z

CRITICAL BUG REPORT FROM USER: "active applications are now unstyled why are they breaking fully working features we already had"

**Root Cause:** In the recent edits to `TaskbarWindow.cpp`, Draftsman-Dan added a massive custom `CDDS_ITEMPREPAINT` block inside `TaskSwitchSubclassProc` to manually call `DrawThemeBackground` using `OpenThemeData(..., L"TaskBand")` and `partId = 1`. This completely botched the native styling for the active taskbar buttons. 

**Action Required Immediately:**
1. **REVERT** that custom `CDDS_ITEMPREPAINT` drawing block in `TaskSwitchSubclassProc`. Do not try to manually draw the taskbar buttons if it breaks their native appearance.
2. If your goal was to make the entire bounding box clickable, you must accomplish that through standard Toolbar hit-testing, margin/padding adjustments (`TB_SETPADDING`), or `WM_LBUTTONDOWN` coordinate forwarding—**not** by hijacking the entire paint cycle and destroying the working native styles.
3. Fix the active application button styles back to how they originally looked.

## Follow-up — 2026-07-09T03:41:05Z

URGENT CLARIFICATIONS AND BUG REPORTS FROM USER:

1. **Misapplied Fix (Clickable Areas):** The user stated: "the entire area not clickable was a problem in the file browser though nothing to do with taskbar". 
   *Action Required:* You applied the clickable area fix to the taskbar buttons, which broke them. You must REVERT the taskbar button edits completely and leave the taskbar alone in this regard. The clickable area fix (making the whole hover area clickable instead of just text) needs to be applied to the **FILE BROWSER** (e.g., Win32Explorer's listview, treeview, or custom controls), NOT the taskbar!

2. **Broken Desktop Background:** The user stated: "no desktop background either what gives so many fully working features."
   *Action Required:* You have somehow broken the desktop background rendering. Check `DesktopWindow.cpp` or the `IShellFolderView` initialization. The custom desktop MUST render the user's native Windows wallpaper. It cannot be black or blank. Figure out what broke the wallpaper rendering and restore the previously working desktop background immediately! DO NOT BREAK EXISTING WORKING FEATURES.

## Follow-up — 2026-07-09T03:41:50Z

EMERGENCY OVERRIDE FROM USER: "no they need to fix what they broke first"

**HALT ALL NEW FEATURE DEVELOPMENT IMMEDIATELY.** 
You are strictly forbidden from working on the file browser clickable areas, the settings CPL transition, or any other new features right now.

Your SOLE and EXCLUSIVE priorities are to fix the regressions you caused:
1. **Restore Native Taskbar Styling:** Completely rip out the custom prepaint cycle that broke the taskbar active applications. Ensure it renders correctly with the native Windows `TaskBand` theme.
2. **Restore Desktop Background:** Fix whatever you broke in `DesktopWindow.cpp` (or the initialization chain) so the native desktop wallpaper renders correctly again instead of being blank/black.

Do not proceed to any other task until these two regressions are resolved and verified.





















