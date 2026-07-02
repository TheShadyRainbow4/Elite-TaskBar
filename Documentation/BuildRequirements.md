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
1. THE STATE MACHINE: This document is your persistent state tracker. You must read the current state of the numbered lists before generating code.

2. ATOMIC EXECUTION: The user will command you to execute a specific Phase or Section. You may ONLY implement the exact granular features requested. DO NOT hallucinate future features. DO NOT bundle untested steps.

3. VERIFICATION LOOP: After generating the code for a specific step or phase, you must HALT execution. Prompt the user to compile and test.

4. UPDATING THE LEDGER: ONLY after the user confirms a successful test, you will edit this document. Append the exact text "[COMPLETED]" to the end of the line for that specific step.

5. THE CHANGELOG: Every time you mark a step as completed, you MUST append an entry to the CHANGELOG at the absolute bottom of this document detailing the exact functions added.

6. NO DESTRUCTIVE REWRITES: Never remove features to fix bugs. Always expand and append.

7. ABSOLUTELY ZERO EMOJIS: Do not generate emojis in the codebase, scripts, or technical documentation.

---
## PHASE 1: ENVIRONMENT SETUP & BOOTSTRAPPER INITIATION
---
1. Create barebones Win32 C++ empty project structure.
2. Define WinMain entry point with standard HINSTANCE and lpCmdLine parameters.
3. Initialize global logging function targeting %SystemDrive%\EliteSoftware\Logs\EliteTaskbar.log.
4. Write bootstrapper logic to append timestamped startup string to the log.
5. Initialize memory leak detection tracking (_CrtSetDbgFlag) for debug builds.
6. Query Windows Registry to determine operational mode ("Independent" vs "Replace").
7. Declare global struct for EliteTaskbar configuration state (Theme, Mode, Monitor Targets).
8. Add structured exception handling (__try / __except) to WinMain to catch fatal crashes.
9. Implement crash dialog MessageBox ("The taskbar has encountered a fatal existence failure.") with hex error codes.
10. Implement manifest logic for Per-Monitor V2 DPI awareness.

---
## PHASE 2: SHELL SPOOFING & NATIVE WINDOW REGISTRATION
---
1. Write function to locate native taskbar: FindWindow("Shell_TrayWnd", NULL).
2. If "Replace" mode: Send ShowWindow(hwnd, SW_HIDE) to hide the native shell.
3. If "Replace" mode: Define WNDCLASSEX with lpszClassName = "Shell_TrayWnd".
4. If "Independent" mode: Define WNDCLASSEX with lpszClassName = "Elite_SecondaryTrayWnd".
5. Register Class using RegisterClassEx. Verify success and log result.
6. Handle TaskbarCreated message: RegisterWindowMessage(L"TaskbarCreated").
7. Instantiate main HWND using CreateWindowEx with WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN.
8. Enforce Z-Order: Call SetWindowPos with HWND_TOPMOST to sit above all maximized apps.
9. Implement standard WindowProc callback loop for the main HWND.
10. Capture WM_DESTROY to cleanly reverse shell hooks and exit.

---
## PHASE 3: MULTI-MONITOR MAPPING & APPBAR RESERVATION
---
1. Implement EnumDisplayMonitors callback function.
2. Retrieve bounding rects (rcMonitor) for all active displays.
3. Filter primary vs secondary monitors based on configuration state.
4. Initialize SHAppBarMessage structures (APPBARDATA).
5. Send ABM_NEW to register the custom taskbar with the Windows Desktop Manager.
6. Calculate height based on DPI scaling (standard legacy height 30px or 40px base).
7. Send ABM_QUERYPOS to request edge space on target monitor.
8. Send ABM_SETPOSITION to lock the workspace edge.
9. Handle WM_DISPLAYCHANGE to trigger re-evaluation of AppBar positions.
10. Handle WM_WINDOWPOSCHANGED to defend AppBar edge space against rogue fullscreen apps.

---
## PHASE 4: WINDOW TRACKING & TASKBAND POPULATION (APP BUTTONS)
---
1. Register global shell hook: RegisterShellHookWindow(hwndMain).
2. Add custom message switch for WH_SHELL in WindowProc.
3. Handle HSHELL_WINDOWCREATED: Extract HWND of new application.
4. Validate HWND (Ignore hidden windows, tooltips, zero-size rects).
5. Allocate custom unmanaged TaskButton struct containing HWND, Title, and HICON.
6. Extract Title: GetWindowText and GetWindowTextLength.
7. Extract Icon: SendMessageTimeout(WM_GETICON, ICON_SMALL).
8. Fallback Icon: GetClassLongPtr(GCLP_HICONSM).
9. Handle HSHELL_WINDOWDESTROYED: Locate struct, free memory, trigger UI redraw.
10. Handle HSHELL_WINDOWACTIVATED: Update "active" state flag on the specific TaskButton.
11. Handle HSHELL_REDRAW: Update title/icon if the application dynamically changes them.
12. Handle HSHELL_FLASH: Implement native window attention flashing logic via GDI timer.
13. Grouping Logic: Implement GetWindowThreadProcessId check to group identical app instances.

---
## PHASE 5: UI METRICS, GDI DRAWING & AERO GLASS IMPLEMENTATION
---
1. Set up double-buffered drawing using CreateCompatibleDC and CreateDIBSection.
2. Implement DWM Aero Glass: Call DwmExtendFrameIntoClientArea (Vista/7 style).
3. Implement colorization polling: DwmGetColorizationColor to tint UI based on system theme.
4. Fallback GDI Renderer: If Aero is disabled/unsupported, implement opaque drawing.
5. Implement 9-slice rastering algorithm using BitBlt for legacy skinning.
6. Draw TaskButton normal state (Native Win32 3D-inset shading or UXTheme).
7. Draw TaskButton hover state.
8. Draw TaskButton active/pressed state.
9. Handle WM_MOUSEMOVE on TaskBand to update hover indices.
10. Handle WM_LBUTTONDOWN: Send ShowWindow(SW_MINIMIZE) or SetForegroundWindow to target.
11. Handle WM_RBUTTONUP: Invoke classic GetSystemMenu and TrackPopupMenuEx.
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
1. Define Start Button hit-box rect at far left of taskbar.
2. Render 3-state Start Button (Normal, Hover, Pressed) from theme atlas.
3. Execute native Start Menu on click via WM_SYSCOMMAND / SC_TASKLIST.
4. Setup 1000ms SetTimer for the Clock.
5. Call GetLocalTime.
6. Format Time via GetTimeFormatEx.
7. Format Date via GetDateFormatEx.
8. Render Clock text with client edge shadow.
9. Show Desktop: Implement DwmEnableBlurBehindWindow for transparent right-edge button.

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
# CHANGELOG & EXECUTION HISTORY
---
* Log format: [Date/Time] - Phase X, Step Y completed. Added [Functions]. 

(Waiting for AI generation...)