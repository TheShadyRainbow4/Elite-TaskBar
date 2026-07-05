# Project: EliteTaskbar & Win32Explorer Integration

## Architecture
The EliteSoftware suite consists of three primary components:
1. **EliteTaskbar.exe** (C++ Win32): Custom desktop shell taskbar.
2. **Win32Explorer.exe** (C++ Win32): Custom file manager and explorer replacement.
3. **EliteSettings.exe** (PowerShell WinForms compiled with PS2EXE) / **EliteSettings.cpl** (C++ Control Panel Applet stub): Unified settings application.

Data flows through registry keys located at `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` (and `HKLM` under Portable Mirror Mode). Custom updates are broadcasted using registered window messages.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | R6: Portable Mirror Mode & R3: Settings Sync | Save config to XML file / HKLM, CPL repair, Replace Explorer toggles and fix stuck logic. | None | DONE |
| 2 | R2: System Tray & R5: Custom Icon Theming | Add system tray icons and Quit option to Taskbar/Explorer. Custom Icon Theme Importer. | M1 | IN_PROGRESS |
| 3 | R1: Deep Metadata & R4: View Modes & Grouping | Display extended metadata (Ratings/Authors/Dimensions) in details. Thumbnail Tiles and Group by Type. | M1 | PLANNED |

## Interface Contracts
### EliteSettings ↔ Win32Explorer / EliteTaskbar
- Settings Registry Key: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`
- In Portable Mirror Mode: Configuration is mirrored to `HKLM\Software\EliteSoftware\Win32Explorer\Advanced` and `config.xml` (located at workspace/executable folder).
- Update Notification Message: `EliteTaskbarSettingsUpdate` registered window message.

## Code Layout
- EliteTaskbar & Settings Stubs: `SourceFiles/`
- Win32Explorer: `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/`
- Output Binaries: `BuildOutput/` and Project Root
