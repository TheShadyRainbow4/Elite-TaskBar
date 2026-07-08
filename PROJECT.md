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
| 1 | Milestone 1: Shell Behavior, Z-Ordering & HKLM Registry Alignment | GetEliteRegistryRoot redirected to HKLM, win32explorer config XML fallback removed, topmost/bottommost window pos locks, fallback Shell_SecondaryTrayWnd. | None | DONE |
| 2 | Milestone 2: Interactive Desktop & Multi-Monitor Support | Custom IShellBrowser hosting IShellView created via SHCreateShellFolderView under registry toggle, secondary desktop monitor backgrounds, start button spoofing. | M1 | DONE |
| 3 | Milestone 3: Independent System Tray & Start Menu | Tray icons routed through common hToolbar, click callbacks subclass, hSysPager tooltips, dynamic tray width calculation. | M2 | DONE |
| 4 | Milestone 4: Settings Completeness & Options Mirroring | Mirror win32explorer options to properties dialog and CPL, Explorer selection/hover modernized. | M3 | DONE |
| 5 | Milestone 5: E2E Verification & Forensic Audit | Compile via build.ps1, E2E tests, Forensic Auditor integrity check. | M4 | DONE |
| 6 | Milestone 6: Resolve ODR Violation & Startup Crash | Resolve ODR collision (MonitorEnumProc) causing crash under Replace mode. | M5 | DONE |
| 7 | Milestone 7: Expand E2E Verification Test Scope & Window Retention | Add topmost lock, tray controls, monitor counts, and HKLM folder options to E2E tests with 5-second sleep pauses. Resolve MonitorInfo struct ODR collision. | M6 | DONE |
| 8 | Milestone 8: Taskbar & System Tray Enhancements | Implement manual tray resizing from active side (with flyout fallback), fix transparency/white bar bug, implement closed program ghost icon refresh mechanism, and support overflow flyout style toggle. | M7 | PLANNED |
| 9 | Milestone 9: Win32Explorer View Modes & Shellbags | Default global view mode to "small tiles" (except "This PC" which uses "full tiles"), default "Group by type" grouping, native shellbags integration for layouts/positions, mirror all non-thumbnail views as new thumbnail-enabled variants. | M7 | PLANNED |
| 10 | Milestone 10: Technical Polish & Okay Dialog Update | Replace all "OK" prompts/dialogs with "Okay" (including exception handling), run E2E verification, and perform Forensic Integrity Audit. | M8, M9 | PLANNED |

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

## Win32Explorer Registry & Configuration Rules
1. **Disable Portable Default**: Win32Explorer must NEVER default to portable mode. It must always default to registry-based configuration.
2. **Concurrent XML Mirroring**: Despite defaulting to the registry, it must concurrently mirror all configuration and settings to the XML file in its local directory.
3. **HKLM Global Registry Only (No HKCU)**: Win32Explorer must save and load ALL of its configuration to/from the master `EliteSoftware` path visible to all users under `HKEY_LOCAL_MACHINE\SOFTWARE\EliteSoftware\Win32Explorer` (or `HKEY_LOCAL_MACHINE\SOFTWARE\EliteSoftware\Win32Explorer\Master`). Usage of per-user `HKEY_CURRENT_USER` configurations is strictly forbidden!

## COM Hijacking & Restoration Rules
1. **Graceful COM Restoration**: In `Win32Explorer.exe` and `EliteTaskbar` graceful exit paths, or when toggling the default file manager settings, all native COM classes (Folder, Directory, CabinetWClass, and other CLSID hijackings) MUST be actively restored and re-registered back to the native `explorer.exe` (no stranded keys allowed).
2. **XML Import Support**: Win32Explorer must support parsing and importing configuration settings from the local XML configuration file.
