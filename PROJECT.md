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
| 7 | Milestone 3: Advanced Desktop & Theme Configuration | Progman multi-display render, Desktop Background tab, Span/Per-monitor, slideshow, default checked tray on all displays, transparent desktop icon backgrounds (no white outlines), toggleable desktop thumbnails, massive Settings UI extension, re-enable CAB backups via build.ps1 utilizing backup.ps1 -BuildOutputOnly. | M3 | IN_PROGRESS |
| 8 | Milestone 4: Global Keyboard Hooks & Shell Fallbacks | Compile OpenShell.sln to EliteStartMenu.exe with -monitor/-rect arguments support (StartMenu.cpp command-line parser), read/inherit from official native Open-Shell registry path, WH_KEYBOARD_LL hooks and RegisterHotKey for Win+R. | M3 | PLANNED |
| 9 | Milestone 5: Taskbar Features & View Modes | Launch EliteStartMenu with monitor arguments, clock seconds, show desktop hover, resizable Quick Launch, view modes. | M3 | PLANNED |
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

## Native Integration & Configuration Guidelines
1. **Registry Integration & Dual-Sync**: Do NOT invent custom registry keys for features that Windows already tracks. Write user configurations to two locations:
   a) The native Windows registry keys (for immediate OS integration).
   b) A custom "Master Configuration" registry key list under `HKCU\Software\EliteSoftware\Win32Explorer\Master` (to store the overall application state).
2. **Boot Synchronization**: Implement a sync loop on startup in `EliteTaskbar` (`main.cpp` / initialization) that reads the custom Master Configuration registry settings, and writes them directly to the real native Windows registry locations (for native features) or loads them internally (for custom features).
3. **Total Configurability**: Ensure that every single component, layout parameter, and engine feature is a toggleable setting in the UI. Do not force or hardcode behaviors; make everything configurable for power users.
4. **Native Classes & Controls**: Use native Win32 window classes/controls (`Progman`, `WorkerW`, `SysListView32`, `ReBarWindow32`, etc.) wherever possible instead of custom rendering or custom classes. Custom classes are reserved ONLY for net-new features that do not exist in standard Windows.

## Dedicated Builder Swarm Workflow
To resolve build lock collisions and race conditions:
1. **Dedicated BUILDER Agent**: A dedicated BUILDER agent sub-orchestrator (`builder_sub_orch`) handles all compilation.
2. **No Direct Builds**: Worker agents are strictly prohibited from running `build.ps1` directly.
3. **Concurrent and Sequential Builds**: The BUILDER agent is authorized to run individual component builds (e.g. `build.ps1 -Target "Win32Explorer"`, etc.) concurrently to speed up compilation. However, the final master build must be queued and executed sequentially at the very end.
4. **Validation Handoff**: Once the build succeeds, control is handed back to the testing/validation swarm for verification.
