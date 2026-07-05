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
