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
