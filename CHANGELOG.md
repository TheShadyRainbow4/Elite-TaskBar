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
