# ELITETASKBAR - MASTER ARCHITECTURAL LEDGER & EXECUTION TRACKER
---
#### Company: EliteSoftwareTech Co.
#### Authors: EliteSoftware - Zachary Whiteman - Susan Gemm
#### Version Target: 1.0.42.6 (Format: Major.Minor.FeatureAdd.BugFix)
#### Target OS: Windows 10 IoT Enterprise LTSC (Legacy compatible: Vista / 7)
#### Core Technologies: Native Unmanaged C++ (Taskbar), .NET Framework 4.6 WinForms (Settings Applet) or Native Windows Functions

---
# CRITICAL AI AGENT DIRECTIVES (READ BEFORE EXECUTING)
---

1. THE STATE MACHINE: This document is your persistent state tracker. You must read the current state of the numbered lists before generating code. Then mark them as complete one by one after each is fully complete. NEVER DELETE ANYTHING FROM THIS DOCUMENT ONLY ADD!

2. ATOMIC EXECUTION: The user will command you to execute a specific Phase or Section. You may ONLY implement the exact granular features requested. DO NOT hallucinate future features. DO NOT bundle untested steps.

3. VERIFICATION LOOP: After generating the code for a specific step or phase, you must HALT execution. Prompt the user to compile and test. 

4. UPDATING THE LEDGER: ONLY after the user confirms a successful test, you will edit this document. Append the exact text "**[COMPLETED]**" to the end of the line for that specific step.

5. THE CHANGELOG: Every time you mark a step as completed, you MUST append an entry to the CHANGELOG at the absolute bottom of this document detailing the exact functions added.

6. NO DESTRUCTIVE REWRITES: Never remove features to fix bugs. Always expand and append.

7. ABSOLUTELY ZERO EMOJIS: Do not generate emojis in the codebase, scripts, or technical documentation.

---
## PHASE 1: ENVIRONMENT SETUP & BOOTSTRAPPER INITIATION
---

[x] 1. Create barebones Win32 C++ empty project structure.

[x] 2. Define WinMain entry point with standard HINSTANCE and lpCmdLine parameters.

[x] 3. Initialize global logging function targeting %SystemDrive%\EliteSoftware\Logs\EliteTaskbar.log.

[x] 4. Write bootstrapper logic to append timestamped startup string to the log.

[x] 5. Initialize memory leak detection tracking (_CrtSetDbgFlag) for debug builds.

[x] 6. Query Windows Registry to determine operational mode ("Independent" vs "Replace").

[x] 7. Declare global struct for EliteTaskbar configuration state (Theme, Mode, Monitor Targets).

[x] 8. Add structured exception handling (__try / __except) to WinMain to catch fatal crashes.

[x] 9. Implement crash dialog MessageBox ("The taskbar has encountered a fatal existence failure.") with hex error codes.

[x] 10. Implement manifest logic for Per-Monitor V2 DPI awareness.

---
## PHASE 2: SHELL SPOOFING & NATIVE WINDOW REGISTRATION
---

1. Write function to locate native taskbar: FindWindow("Shell_TrayWnd", NULL). **[COMPLETED]**

2. If "Replace" mode: Send ShowWindow(hwnd, SW_HIDE) to hide the native shell. **[COMPLETED]**

3. If "Replace" mode: Define WNDCLASSEX with lpszClassName = "Shell_TrayWnd". **[COMPLETED]**

4. If "Independent" mode: Define WNDCLASSEX with lpszClassName = "Elite_SecondaryTrayWnd". **[COMPLETED]**

5. Register Class using RegisterClassEx. Verify success and log result. **[COMPLETED]**

6. Handle TaskbarCreated message: RegisterWindowMessage(L"TaskbarCreated"). **[COMPLETED]**

7. Instantiate main HWND using CreateWindowEx with WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN. **[COMPLETED]**

8. Enforce Z-Order: Call SetWindowPos with HWND_TOPMOST to sit above all maximized apps. **[COMPLETED]**

9. Implement standard WindowProc callback loop for the main HWND. **[COMPLETED]**

10. Capture WM_DESTROY to cleanly reverse shell hooks and exit. **[COMPLETED]**

11. Replace Mode Subversion (UIPI Defense): Implement a background timer loop that teleports the hidden native shell off-screen to `(-10000, -10000)`. This prevents the legacy system tray or clock from rendering through the Aero transparency frame if UIPI blocks non-elevated concealment.
---
## PHASE 3: MULTI-MONITOR MAPPING & APPBAR RESERVATION
---

1. Implement EnumDisplayMonitors callback to capture primary screen dimensions. **[COMPLETED]**

2. Retrieve bounding rects (left, top, right, bottom). **[COMPLETED]**

3. Filter out secondary monitors if "Multi-Mon" mode is disabled in config. **[COMPLETED]**

4. Initialize SHAppBarMessage. **[COMPLETED]**

5. Populate APPBARDATA struct with calculated taskbar height and uEdge = ABE_BOTTOM. **[COMPLETED]**

6. Send ABM_NEW to register as an AppBar. **[COMPLETED]**

7. Send ABM_QUERYPOS to ensure Explorer clears the space. **[COMPLETED]**

8. Send ABM_SETPOS to finalize our coordinates. **[COMPLETED]**

9. Handle WM_DISPLAYCHANGE to trigger re-evaluation of AppBar positions.

10. Handle WM_WINDOWPOSCHANGED to defend AppBar edge space against rogue fullscreen apps.

11. Decoupled Geometry Matrix: Completely bypass global singletons like `SM_CXSCREEN`. Leverage `EnumDisplayMonitors` alongside `GetMonitorInfoW` to harvest `rcMonitor` and `rcWork` limits.

12. Independent Screen Allocation: Dynamically spawn a dedicated, independent `HWND` handle per screen, binding an isolated geometry structure instance containing local monitor boundaries to each window.
---
## PHASE 4: WINDOW TRACKING & TASKBAND POPULATION (APP BUTTONS)
---

1. Register global shell hook: RegisterShellHookWindow(hwndMain). **[COMPLETED]**

2. Add custom message switch for WH_SHELL in WindowProc. **[COMPLETED]**

3. Handle HSHELL_WINDOWCREATED: Extract HWND of new application. **[COMPLETED]**

4. Validate HWND (Ignore hidden windows, tooltips, zero-size rects). **[COMPLETED]**

5. Allocate custom unmanaged TaskButton struct containing HWND, Title, and HICON. **[COMPLETED]**

6. Extract Title: GetWindowText and GetWindowTextLength. **[COMPLETED]**

7. Extract Icon: SendMessageTimeout(WM_GETICON, ICON_SMALL). **[COMPLETED]**

8. Fallback Icon: GetClassLongPtr(GCLP_HICONSM). **[COMPLETED]**

9. Handle HSHELL_WINDOWDESTROYED: Locate struct, free memory, trigger UI redraw. **[COMPLETED]**

10. Handle HSHELL_WINDOWACTIVATED: Update "active" state flag on the specific TaskButton. **[COMPLETED]**

11. Handle HSHELL_REDRAW: Update title/icon if the application dynamically changes them. **[COMPLETED]**

12. Handle HSHELL_FLASH: Implement native window attention flashing logic via GDI timer.

13. Grouping Logic: Implement GetWindowThreadProcessId check to group identical app instances.

14. Shell Hook Message ID: Retrieve the unique system message identifier via `UINT g_uShellHookMsg = RegisterWindowMessageW(L"SHELLHOOK");` and use this in the message pump.

15. Strict Window Filtering (HSHELL_WINDOWCREATED): Implement a triple-filter validation check: 1) `IsWindowVisible(hwnd)` must be TRUE, 2) `GetWindowLongPtrW(hwnd, GWL_EXSTYLE)` must not contain `WS_EX_TOOLWINDOW`, and 3) `GetWindow(hwnd, GW_OWNER)` must return `NULL`.
---
## PHASE 5: UI METRICS, GDI DRAWING & AERO GLASS IMPLEMENTATION
---

1. Set up double-buffered drawing using CreateCompatibleDC and CreateDIBSection.

2. Implement DWM Aero Glass: Call DwmExtendFrameIntoClientArea (Vista/7 style). **[COMPLETED]**

3. Implement colorization polling: DwmGetColorizationColor to tint UI based on system theme.

4. Fallback GDI Renderer: If Aero is disabled/unsupported, implement opaque drawing. **[COMPLETED]**

5. Implement 9-slice rastering algorithm using BitBlt for legacy skinning.

6. Draw TaskButton normal state (Native Win32 3D-inset shading or UXTheme). **[COMPLETED]**

7. Draw TaskButton hover state.

8. Draw TaskButton active/pressed state.

9. Handle WM_MOUSEMOVE on TaskBand to update hover indices.

10. Handle WM_LBUTTONDOWN: Send ShowWindow(SW_MINIMIZE) or SetForegroundWindow to target.

11. Handle WM_RBUTTONUP: Invoke classic GetSystemMenu and TrackPopupMenuEx. **[COMPLETED]**

12. Font Rendering: CreateFontIndirect targeting Montserrat Semibold, fallback to Segoe UI Semibold.

---
## PHASE 6: THE SYSTEM TRAY (NOTIFICATION AREA) PARSING
---

1. Create child window class "TrayNotifyWnd" inside the main taskbar.

2. Hook Shell_NotifyIcon to intercept NOTIFYICONDATA structures.

3. Handle NIM_ADD: Allocate TrayIcon struct (HICON, Tooltip, uCallbackMessage, HWND).

4. Handle NIM_MODIFY: Update specific HICON or Tooltip text safely.

5. Handle NIM_DELETE: Remove icon from array, recalculate horizontal layout.

6. Render 16x16 tray icons using DrawIconEx.

7. Implement WM_MOUSEMOVE tooltips using CreateWindowEx(TOOLTIPS_CLASS).

8. Handle Left Click on Tray Icon: Route uCallbackMessage with WM_LBUTTONDOWN.

9. Handle Right Click on Tray Icon: Route uCallbackMessage with WM_RBUTTONDOWN.

---
## PHASE 7: TRAY FLYOUT & CHEVRON LOGIC
---

1. Read configuration for Tray Mode ("Vista Inline" vs "Windows 7 Flyout").

2. Vista Mode: Calculate width of visible icons vs hidden icons.

3. Vista Mode: Render classic chevron expander button (<).

4. Vista Mode: Implement animation timer to smoothly slide chevron and expand tray area.

5. Win7 Mode: Create separate popup window class for the decoupled flyout overflow.

6. Win7 Mode: Route NIM messages into the popup HWND when chevron is clicked.

7. Balloon Tips: Hook NIIF_INFO flags to draw custom legacy yellow balloon tooltips.

---
## PHASE 8: COM HOSTING & DESKBAND TOOLBARS
---

1. Initialize COM via CoInitializeEx(COINIT_APARTMENTTHREADED).

2. Define IObjectWithSite implementation in unmanaged C++.

3. Define IOleWindow and IDeskBand wrapper structures.

4. Instantiate Rebar control (INITCOMMONCONTROLSEX, ICC_COOL_CLASSES).

5. Load external IDeskBand CLSIDs via CoCreateInstance.

6. Route desk band child window output into the Rebar container.

7. Quick Launch: Parse IShellLink shortcuts from user directory.

8. Extract IShellLink icons and targets.

9. Draw legacy dotted gripper handles for resizing toolbars.

10. Implement IDropTarget on Quick Launch to allow file dragging (ensure non-elevated).

---
## PHASE 9: START BUTTON, CLOCK & CONTROLS
---

1. Define Start Button hit-box rect at far left of taskbar. **[COMPLETED]**

2. Render 3-state Start Button (Normal, Hover, Pressed) from theme atlas. **[COMPLETED]**

3. Execute native Start Menu on click via WM_SYSCOMMAND / SC_TASKLIST. **[COMPLETED]**

4. Setup 1000ms SetTimer for the Clock. **[COMPLETED]**

5. Call GetLocalTime. **[COMPLETED]**

6. Format Time via GetTimeFormatEx. **[COMPLETED]**

7. Format Date via GetDateFormatEx. **[COMPLETED]**

8. Render Clock text with client edge shadow. **[COMPLETED]**

9. Show Desktop: Implement DwmEnableBlurBehindWindow for transparent right-edge button. **[COMPLETED]**

10. Show Desktop: Trigger keybd_event VK_LWIN + D on click. **[COMPLETED]**

---
## PHASE 10: ELITESOFTWARE SETTINGS APPLET (.NET FRAMEWORK 4.6)
---

1. Create separate C# WinForms .NET 4.6 project (EliteTaskbar_Config.exe).

2. Set Form BorderStyle to FixedDialog or SizableToolWindow (Classic Native Win32 feel).

3. Apply Montserrat Semibold font to all labels, textboxes, and checkboxes.

4. Configure buttons with text: "Done", "Next", "Cancel", "Exit", "Okay", "Apply".

5. Verify ALL buttons have no BackColor set (Must use OS EnableVisualStyles).

6. Create CheckBox matrix for Monitor Selection (Monitor 1, Monitor 2, etc.).

7. Create Radio Buttons for "Independent" vs "Replace Native Shell".

8. Create ComboBox for Legacy Theme Selection.

9. Add System.Windows.Forms.ToolTip to every interactive element.

10. Write witty/intellectual tooltip strings describing each setting.

---
## PHASE 11: INTER-PROCESS COMMUNICATION (IPC) & POLISH
---

1. Define custom WM_COPYDATA struct for passing settings.

2. Applet: Send WM_COPYDATA to EliteTaskbar HWND on "Apply" click.

3. Taskbar: Catch WM_COPYDATA, parse new struct, invalidate UI rects, and redraw.

4. Implement logic to write settings to the registry or custom .ini.

5. Create "View EliteTaskbar Logs" link in the WinForms applet pointing to notepad.exe %SystemDrive%\EliteSoftware\Logs\EliteTaskbar.log.

6. Conduct full memory leak sweep using CRT debug heap.

7. Convert Settings Dialog to save/read standard Windows features (Lock Taskbar, Auto-Hide, Small Icons) from native Registry locations (`HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced` and `StuckRects3`).

8. Implement a toggle in the Custom Settings Property Sheet to switch between Elite's Custom Feature implementations and the real Native Registry Mode, preserving the combined UI.

9. Refactor `TaskbarWindow` architecture to instantiate dynamically across an array of `HWND` objects to populate additional identical taskbars on all secondary monitors detected by `EnumDisplayMonitors`.

---
## RESOURCE ASSETS
---
* **Start Button Orbs (StartIsBack style):**
  * `Resources\StartOrb\aqua bottom.png`
  * `Resources\StartOrb\cyan.png`
  * `Resources\StartOrb\dunes.png`
  * `Resources\StartOrb\indigo.png`
  * `Resources\StartOrb\orb.png`
  * `Resources\StartOrb\sapphire.png`
  * `Resources\StartOrb\uranus.png`
  * `Resources\StartOrb\vienna bottom.png`
* **Icons:**
    * Main Program Icon: `Resources\MAIN_PROGRAM.ico`
    * Preferences Dialog Window Icon: `Resources\POREFERENCES.ico`

---
# LIII. NATIVE FLYOUT INVOCATION & SYSPAGER ROUTING
---

1. Volume Slider Invocation: Intercept the click event on the custom SysPager audio icon. Execute CreateProcessW targeting "sndvol.exe -f [X] [Y]", dynamically passing the bounding RECT coordinates of the custom taskbar icon to anchor the legacy mixer.

2. Network Pane Routing: Intercept the network icon click event. Utilize ShellExecute or CreateProcessW to invoke "rundll32.exe van.dll,RunVAN" to spawn the native unmanaged network selection UI.

3. Clock Click Action: Map the primary clock click event to execute "control.exe timedate.cpl" as a robust Win32 fallback, bypassing the missing legacy TrayClockWClass rendering code in modern LTSC builds.


4. ITrayTask COM Integration (Coexistence Mode): Define the undocumented ITrayTask interface (IID: FB2DC9CE-A8FA-44D9-8219-C4A18A222DDF). If ELITE_STATE_COEXISTENCE is active, use CoCreateInstance to bind to the native shell and command flyout visibility via the ShowClock() and ShowVolume() vtable methods.

---
# XI. DESKTOP WINDOW ROUTING & ICON GRID (PROGMAN)
---

1. Desktop Window Registration: Register and spawn a borderless, bottom-Z-order window using the class name "Progman". **[COMPLETED]**
2. Wallpaper Rendering: Intercept WM_ERASEBKGND or WM_PAINT on the Progman HWND. Read the wallpaper path from HKCU\Control Panel\Desktop\Wallpaper and use SystemParametersInfo(SPI_GETDESKWALLPAPER) to render the image using GDI BitBlt or StretchBlt. **[COMPLETED]**
3. Icon Grid Initialization: Create a child window of Progman using the "SHELLDLL_DefView" class, and within that, initialize a "SysListView32" common control. **[COMPLETED]**
4. Directory Binding: Populate the SysListView32 by enumerating the IShellFolder interface for CSIDL_DESKTOPDIRECTORY and CSIDL_COMMON_DESKTOPDIRECTORY. **[COMPLETED]**
5. File System Watcher: Implement SHChangeNotifyRegister to catch file creations/deletions on the desktop and automatically refresh the SysListView32 grid. **[COMPLETED]**

---
# XII. GLOBAL HOTKEYS & SHELL SERVICES
---

1. Hotkey Registration: During bootstrapper init, call RegisterHotKey for vital shell combinations including Win+R, Win+E, and Win+D.
2. Run Dialog Invocation: In the hotkey message loop, intercept Win+R and dynamically invoke the RunFileDlg function from shell32.dll.
3. Recycle Bin Polling: Hook SHQueryRecycleBin to update the desktop Recycle Bin icon state (Empty vs. Full) upon initialization and file deletion events.

---
# XIII. JUMP LIST & CONTEXT MENU EVENT ROUTING
---

1. Shift-State Detection: In the taskband WindowProc, intercept WM_RBUTTONUP and call GetAsyncKeyState(VK_SHIFT).
2. Native Legacy Menu (Shift Held): If VK_SHIFT is active, query GetSystemMenu(target_hwnd, FALSE) and render the classic window management context menu via TrackPopupMenuEx.
3. Jump List Invocation (Shift Released): If VK_SHIFT is not active, execute the IApplicationDocumentLists COM extraction and render the custom Jump List UI.

---
# XIV. REDUNDANT CONFIGURATION APPLET (STANDALONE)
---

1. Fallback Config UI: Ensure the EliteTaskbar_Config.exe WinForms applet contains explicit redundant controls for native shell properties (Taskbar location, Auto-hide, Icon size).
2. Registry Sync: Map the custom config UI to read/write directly to the native HKCU\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced keys to ensure parity with the native dialogs.

---
# XV. NATIVE RUN DIALOG INVOCATION & SCREEN CENTERING
---

1. Dynamic Linkage: Use LoadLibrary("shell32.dll") and GetProcAddress with ordinal 61 to locate the undocumented RunFileDlg function.
2. Global Hotkey Routing: In the main WindowProc, catch the WM_HOTKEY message for Win+R to trigger the Run dialog launch sequence.
3. Centering Hook Setup: Before calling RunFileDlg, install a thread-local WH_CBT hook using SetWindowsHookEx to intercept the dialog's creation frame.
4. Coordinate Math: In the CBT hook callback (HCBT_ACTIVATE), intercept the Run dialog HWND. Calculate the screen center coordinates using GetSystemMetrics(SM_CXSCREEN) and SM_CYSCREEN, and apply a custom vertical offset to pull it slightly closer to the taskbar.
5. Teleportation: Use SetWindowPos within the CBT hook to move the dialog to the calculated coordinates before the window is fully painted.
6. Execution & Cleanup: Call the RunFileDlg function pointer passing the Progman HWND (or NULL) as the owner. Immediately unhook the CBT hook after the dialog is closed to prevent memory leaks.

---
# XVI. ELITE SHELL WRAPPERS (STANDALONE DLL COMPONENT)
---

1. Project Initialization: Create a new unmanaged C++ Dynamic Link Library project named "EliteShellWrappers".
2. RunDLL32 Compatibility: Define exported functions using the standard CALLBACK signature (HWND, HINSTANCE, LPSTR, int) to allow execution via rundll32.exe.
3. Module Linkage: Implement safe LoadLibrary("shell32.dll") calls within each wrapper function to dynamically resolve target APIs at runtime.
4. Wrapper 1 (Shutdown): Map GetProcAddress to ordinal 60 to invoke SHShutDownDialog.
5. Wrapper 2 (Format): Map GetProcAddress to "SHFormatDrive" and parse the lpszCmdLine string to determine the target drive letter.
6. Wrapper 3 (Open With): Map GetProcAddress to "OpenAs_RunDLL" and pass the unassociated file path from lpszCmdLine.
7. Wrapper 4 (About): Map GetProcAddress to "ShellAboutW", injecting "EliteSoftware Win32Explorer" as the szApp parameter to customize the dialog branding.

---
# XVII. ELITE DLL EXPORT SCANNER (STANDALONE UTILITY)
---

1. Initialization: Create a new WinForms or Win32 GUI executable (EliteDllScanner.exe) with a File Picker button, a ListBox for exports, and an Execution button.
2. PE Header Parsing: Implement a file reader that maps the target DLL into memory (using CreateFileMapping and MapViewOfFile) with READ_ONLY access.
3. Export Table Extraction: Traverse the DOS and NT headers to locate the IMAGE_DIRECTORY_ENTRY_EXPORT data directory.
4. Name Enumeration: Iterate through the AddressOfNames array within the export directory, extracting each function name string and populating the UI ListBox.
5. Execution Routing: On user execution trigger, format a command string dynamically: "rundll32.exe [DLL_Path],[Selected_Export] [Optional_Args]".
6. Launch: Pass the formatted command string to CreateProcess or ShellExecute to safely test the undocumented entry point.

---
# XVIII. DUAL-NATURE MONOLITHIC BUILD SYSTEM
---

1. Preprocessor Configuration: Define a global compiler flag (e.g., /D ELITE_MONOLITHIC_BUILD) in the build script to toggle static linking versus dynamic DLL loading.
2. Component Embedding: Update the resource script (.rc) to include external dependencies (e.g., OpenWithEx logic, Everything portable service) as RCDATA binary resources when the monolithic flag is active.
3. Runtime Extraction Logic: If compiled monolithically, implement a bootstrapper function in WinMain that extracts embedded RCDATA tools to %TEMP%\EliteShell\ and executes them dynamically.

---
# XIX. FALLBACK START MENU (OPEN-SHELL & REACTOS INTEGRATION)
---

1. Source Assimilation: Extract the core Start Menu rendering logic from the Open-Shell (C++) repository, isolating the skinning engine and ItemList rendering.
2. Fallback Hook: If FindWindow("Button", "Start") for the native Start Menu fails, initialize the assimilated Open-Shell rendering class and bind it to the EliteTaskbar Start Orb HWND.
3. WinPE Routing: Map the Start Menu shortcut parsing logic to a local relative directory (.\StartMenu_PE) when the native %ProgramData% paths are unavailable.

---
# XXI. OPEN-SHELL UNMANAGED C++ ASSIMILATION & THEMING
---

1. Repository Integration: Clone the Open-Shell unmanaged C++ source tree and isolate the StartMenu and ClassicExplorer (Toolbar) modules.
2. Skinning Engine Implementation: Port the Open-Shell skin parser to read .skin and .skin7 files from a local, portable directory (e.g., .\Skins\).
3. Native Override: Wire the Start Button click event to natively spawn the assimilated Open-Shell menu class as the primary UI, discarding the Windows 10/11 modern menu entirely.
4. Configuration UI Mapping: Map the Open-Shell configuration variables directly into the master EliteTaskbar Settings Dialog to allow runtime customization of menu columns, icon sizes, and glass opacity.

---
# XXII. POWERSHELL 7 DUAL-COMPILE ORCHESTRATION
---

1. Script Initialization: Create Build-EliteSuite.ps1 utilizing PowerShell 7 syntax and strict error handling (Try/Catch blocks).
2. Build Logging: Configure the script to append all MSBuild standard output and error streams to %SystemDrive%\EliteSoftware\Logs\Build-EliteSuite.log, prefixing entries with timestamps.
3. Target Matrix Definition: Define a build matrix array containing the target names and their corresponding preprocessor directives (e.g., "Monolithic" -> "ELITE_MONOLITHIC", "Taskbar" -> "ELITE_STANDALONE_TASKBAR", "Dialogs" -> "ELITE_STANDALONE_DIALOGS").
4. Icon Dynamic Swapping: For each standalone pass, instruct the MSBuild command to target the specific .ico file matching the tool's intended identity via resource compiler flags.
5. Version Stamping: Ensure the script injects the 4-decimal version string (e.g., 1.0.0.0) and "EliteSoftware - Zachary Whiteman - Susan Gemm" author metadata into the compiled headers of all outputs.

---
# XXIII. ELITE PROPERTIES DIALOG & DYNAMIC PROPERTY SHEETS
---

1. Resource Definition: Define dialog templates in the .rc file for each configuration page (Taskbar, Start Menu, Desktop, Toolbars) enforcing the DS_CONTROL | WS_CHILD styles required for property pages.
2. Property Sheet Construction: Implement a C++ function that initializes an array of HPROPSHEETPAGE handles using CreatePropertySheetPage for each standard configuration template.
3. Command Line Routing: Parse lpCmdLine during initialization. Map arguments (e.g., "/tab:desktop") to the psh.nStartPage integer within the PROPSHEETHEADER structure to mimic the native Options_RunDLL behavior.
4. Secret Tab Authentication: Query HKCU\Software\EliteSoftware\Win32Explorer\Advanced for the DWORD "EnableDebugTabs". Also parse lpCmdLine for the "/devmode" switch.
5. Dynamic Tab Injection: If authentication passes, dynamically allocate and append the HPROPSHEETPAGE handles for the "Everything Indexer" and "DLL Scanner" configuration templates to the primary array before invoking PropertySheet().

---
# XXIV. RESOURCE ICON MULTIPLEXING & WINDOW BRANDING
---

1. Resource Allocation: Define an array of distinct .ico files within the Win32Explorer.rc file, assigning sequential integer IDs (e.g., IDI_MAIN=101, IDI_TASKBAR=102, IDI_SETTINGS=103).
2. Window Class Branding: Intercept the PSCB_INITIALIZED callback within the Property Sheet initialization routine.
3. Message Injection: Execute LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SETTINGS)) and send WM_SETICON (ICON_BIG and ICON_SMALL) to the Property Sheet HWND to override the inherited process icon.

---
# MILESTONE 5: ADVANCED SHELL INTEGRATION & TRAY FIXES
---

1. Start Menu & Flyout Spawning Fix (Display Spoofing): Implement dynamic primary display spoofing, g_IsSpoofingNativeTaskbar / g_SpoofStartTime flags, StartNativeTaskbarSpoof helper, and timer logic. Under subclassed start button click and tray subclassed uCallbackMessage, call StartNativeTaskbarSpoof.
2. Clock Gap Fix: Adjust W_clock to MulDiv(85, dpi, 96) and update text alignment to center-aligned without offset.
3. Tray Icons Fixes: Implement dynamic Windows 10 (offset 24) vs Windows 11 (offset 16) TRAYDATA probing using GetIconInfo. Fallback to GetWindowIconFix / GCLP_HICONSM. Subclass hSysPager to remove white background using DrawThemeParentBackground.
4. Two-Row Tray Option: Define IDC_TWO_ROW_TRAY (294), add checkbox control in rc/Properties dialog (shift groupbox down by 15), config state EnableTwoRowTray, custom layout step spacing, wrapper toolbar, and draw / hit-test tray icons in two rows.
5. Tray Actions: Define 3 callback messages, register 3 custom tray icons (Win32Explorer, Taskbar, Desktop) on primary taskbar, implement single/double-click callbacks and context menus.

---
# CHANGELOG & EXECUTION HISTORY
---

* Log format: [Date/Time] - Phase X, Step Y completed. Added [Functions]. 

[2026-07-02 13:50] - Phase 2, Steps 1-10 completed. Added [WindowProc, TaskbarWindow::Initialize, TaskbarWindow::RunMessageLoop, TaskbarWindow::Cleanup, FindWindowW, ShowWindow, RegisterWindowMessageW].
[2026-07-02 13:52] - Phase 9, Steps 1-3 completed. Added [StartButton::Initialize, StartButton::Draw, WM_PAINT, WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, GDI+ integration].
[2026-07-02 13:55] - Phase 9, Steps 4-5 completed. Added [ClockWidget::Draw, TrayClockProc, WM_TIMER, WM_PAINT, SetTimer, KillTimer].
[2026-07-02 13:58] - Phase 9, Steps 8-10 completed. Added [TrayShowDesktopButtonProc, VK_LWIN simulation].
[2026-07-02 14:03] - Phase 5, Step 11 completed. Added [WM_RBUTTONUP, CreatePopupMenu, AppendMenuW, TrackPopupMenuEx, WM_COMMAND handling for Exit, Task Manager, Explorer].
[2026-07-02 14:12] - Phase 5, Steps 2, 4, 6 completed. Added [DwmExtendFrameIntoClientArea, OpenThemeData, DrawThemeBackground].
[2026-07-02 14:24] - Bug Fixes for Phase 9 & 5 completed. Refactored StartButton into a dedicated WS_EX_LAYERED Topmost floating window to support StartIsBack style desktop overhang. Implemented DIBSection and 32-bit Alpha rendering for GDI+ orb strip animation. Fixed VK_LWIN Start Menu hooks for OpenShell support. Fixed ClockWidget ClearType alpha-channel destruction. Fixed Tray child windows DWM Glass compatibility.
---
# MISSING NATIVE FEATURES & PENDING REQUESTS
---

1. **Full Deskband Support & Folders**: Native implementation of QuickLaunch, toolbars, deskbands and folder toolbars, allowing different toolbars per taskbar monitor.
2. **Explorer Integration**: File browser compatibility must remain fully functional while running as shell.
3. **Native Windows 7 Flyout Mode**: Restore and integrate native Windows 7 Notification Area flyout UI functionality.
4. **Legacy Tray Mode**: Implement ReactOS-style classic linear tray logic as an alternate selectable mode.
5. **Settings Menu Expansion**: Populate all tabs with all settings, implementing switching between OpenShell, Native Menu, and custom combinations based on triggers.
[2026-07-03 02:26] - Phase 4, Steps 1-11 completed. Added [TaskButtonInfo, RegisterShellHookWindow, HSHELL_WINDOWCREATED, HSHELL_WINDOWDESTROYED, HSHELL_WINDOWACTIVATED, HSHELL_REDRAW, Taskbar buttons rendering with Vista style wide labels and small icons, StartButton unconditionally created on all monitors].
[2026-07-03 04:27] - Fixed background rendering (Taskbar entirely white bug). Resolved Start Menu multi-monitor spawning with PostMessageW (SC_TASKLIST). Implemented dynamic monitor sync for taskbar buttons. Added Everything SDK flyout implementation.
[2026-07-04 02:35] - Merged Win32Explorer and EliteTaskbar into a single executable process. Resolved duplicate resource bindings and configured the standalone build script to correctly output to root dir.
[2026-07-04 20:21] - Milestone 1 (R6: Portable Mirror Mode & R3: Settings Synchronization & CPL Repair) completed. Added [GetEliteRegistryRoot, SetDefaultFileManagerCPP, Portable Mirror checkbox in C++ and PowerShell Settings, Explorer Replacement radio buttons, CPL build chain repair, unconditional shell registry cleanup on None revert].
[2026-07-05 02:26] - Milestone 3 (R4: Double Scraping Overflow, R5: UWP Icon Getter, R7: Multi-Monitor WM_DPICHANGED DPI Scaling, R8: About Dialog Expanded Height Fix, R9: Settings Thread Timeout & PowerShell Reboot) completed. Added [ScrapeTrayIconsFromToolbar, ScrapeToolbar, GetScrapedTrayTooltip, ExtractWindowIcon, WM_DPICHANGED, BroadcastSettingsChangeThread, IDC_IMPORT_SETTINGS, IDC_EXPORT_SETTINGS registry handlers, _wfopen_s].
[2026-07-05 03:30] - Milestone 3 Remediation completed. Added [DDF backup optimizations, resource ID collision fixes on 234/235, About Dialog layout spacing reduction to 110/195 DUs, Property Sheet button tooltips subclassing on WM_SHOWWINDOW, Settings apply restart folder path resolution via g_hInstance].
[2026-07-05 07:46] - Win32Explorer View Mode, Grouping, and Test Sync Remediation completed. Added case IDM_VIEW_SMALLICONTILES routing, startup showInGroups check with MoveItemsIntoGroups(), robust BM_SETCHECK and SetForegroundWindow in run_re_verification.ps1.
[2026-07-05 16:38] - E2E Testing Track completed. Added [verify_final_polish.ps1, run_comprehensive_e2e.ps1, E2E test suite covering 10 features across 4 tiers].
[2026-07-05 16:42] - PROJECT.md Milestone 4 marked as DONE. Added [milestone update, changelog update].

---
## MILESTONE 5 FEEDBACK FIXES (CURRENT WORK)
---

1. Tray Item Limit: Define TRAY_LIMIT 48 and replace hardcoded 4 limit in all checks, including drawn count overflow checks in TrayFlyoutProc. **[COMPLETED]**
2. Missing Icons (Scraping Fallback): Implement GetProcessIcon fallback using QueryFullProcessImageNameW and SHGetFileInfoW, and use it in GetWindowIconFix and ScrapeTrayIconsFromToolbar. **[COMPLETED]**
3. White Background Bar: Call SetWindowTheme(hSysPager, L"", L"") and SetWindowTheme(hToolbar, L"", L"") to strip native visual style themed backgrounds. Implement DrawThemeParentBackground and custom parent background/WM_ERASEBKGND drawing in SysPagerSubclassProc and TrayToolbarSubclassProc subclass procedures. **[COMPLETED]**

[2026-07-05 16:48] - Milestone 5 Feedback Fixes completed. Added [TRAY_LIMIT, GetProcessIcon, SetWindowTheme calls on hSysPager and hToolbar, WM_ERASEBKGND handling in TrayToolbarSubclassProc].

---
## MILESTONE 5 POLISH & ROBUSTNESS FIXES (CURRENT WORK)
---

1. GDI HICON Leak Fix: Add `bool bOwnsIcon` to `ScrapedTrayIcon` struct, track owned status in `GetWindowIconFix`, fallback to shared system application icon when NULL, and destroy owned icons in `UpdateTrayToolbar` and `TrayNotifyProc`.
2. Primary Monitor Display Spoofing: Remove monitor check so `StartNativeTaskbarSpoof` is called unconditionally on all clicks.
3. Toolbar Hover State: Preserve the glossy hover state of the toolbar buttons by not calling `SetWindowTheme` on `hToolbar`.
4. Active Items Alignment: Dynamically query `hTaskSwitch` button height using `TB_GETBUTTONSIZE` and center the task switch control vertically.
5. Clock Alignment: Center multi-line clock text vertically in `TrayClockProc`'s `WM_PAINT` handler using `DrawTextW` with `DT_CALCRECT`.
6. Build & Test Fixes: Wrap the `EliteStartMenu.ps1` compilation in a `Test-Path` check, and fix E2E verification variables `$hwndNotify` and `$hwndClock` in `run_comprehensive_e2e.ps1`.

