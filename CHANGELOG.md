# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]
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
