# Project: EliteTaskbar & Win32Explorer Integration

## Architecture
The EliteSoftware suite consists of three primary components:
1. **EliteTaskbar.exe** (C++ Win32): Custom desktop shell taskbar.
2. **Win32Explorer.exe** (C++ Win32): Custom file manager and explorer replacement.
3. **EliteSettings.exe** (C++ Win32) / **EliteSettings.cpl** (C++ Control Panel Applet stub): Unified settings application.

Data flows through registry keys located at `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` (and `HKLM` under Portable Mirror Mode). Custom updates are broadcasted using registered window messages.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | R6: Portable Mirror Mode & R3: Settings Sync | Save config to XML file / HKLM, CPL repair, Replace Explorer toggles and fix stuck logic. | None | DONE |
| 2 | R2: System Tray & R5: Custom Icon Theming | Add system tray icons and Quit option to Taskbar/Explorer. Custom Icon Theme Importer. | M1 | DONE |
| 3 | R1: Deep Metadata & R4: View Modes & Grouping | Display extended metadata (Ratings/Authors/Dimensions) in details. Thumbnail Tiles and Group by Type. | M1 | DONE |
| 4 | E2E Testing Track | Design and implement comprehensive E2E test suite; write verify_final_polish.ps1 and compile tests. | None | DONE |
| 5 | Milestone 1: Multi-Monitor & Tray/Flyout Fixes | Dynamic primary display spoofing, clock/tray gap, two-row tray, tray actions. | M3 | DONE |
| 6 | Milestone 2: Settings UI, About Dialog, & Migration Cleanup | Start tab hover bug, About dialog close buttons, reload explorer window bug, *old*.exe cleanup. | M3 | DONE |
| 7 | Milestone 3: Advanced Desktop & Theme Configuration | Progman multi-display render, Desktop Background tab, Span/Per-monitor modes, Slideshow, dropdown theme select. | M3 | IN_PROGRESS |
| 8 | Milestone 4: Global Keyboard Hooks & Shell Fallbacks | WH_KEYBOARD_LL hooks and RegisterHotKey for Win+R run dialog fallback when Explorer is killed. | M3 | PLANNED |
| 9 | Milestone 5: Taskbar Features & View Modes | Clock seconds, show desktop hover, resizable Quick Launch, Win32Explorer view modes. | M3 | PLANNED |
| 10| Final Milestone: Pass E2E Tests & Hardening | Run all E2E tests, debug and fix failures, run white-box adversarial testing. | M4, M5, M6, M7, M8, M9 | PLANNED |

## Interface Contracts
### EliteSettings ↔ Win32Explorer / EliteTaskbar
- Settings Registry Key: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`
- In Portable Mirror Mode: Configuration is mirrored to `HKLM\Software\EliteSoftware\Win32Explorer\Advanced` and `config.xml` (located at workspace/executable folder).
- Update Notification Message: `EliteTaskbarSettingsUpdate` registered window message.

## Code Layout
- EliteTaskbar & Settings Stubs: `SourceFiles/`
- Win32Explorer: `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/`
- Output Binaries: `BuildOutput/` and Project Root
