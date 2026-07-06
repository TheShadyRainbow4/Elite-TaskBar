# Scope: Milestone 5 - Core Shell Extensions and Tray Polish

## Architecture
- Target files for Milestone 5 implementation:
  - `SourceFiles/TaskbarWindow.cpp` & `SourceFiles/TaskbarWindow.h`: Core layout, clock, primary display spoofing, tray click action routing, and tray icon drawing.
  - `SourceFiles/TrayIconScraper.cpp` & `SourceFiles/TrayIconScraper.h`: System tray parsing, toolbar rendering, and 2-row tray configuration.
  - `SourceFiles/TaskbarProperties.cpp`: settings toggles logic (2-Row Tray mode).
  - `SourceFiles/resource.h`, `SourceFiles/resources.rc`, `SourceFiles/settings_resources.rc`: resources layout, properties UI elements, and dialog layouts.
  - `SourceFiles/main.cpp`: initialization and argument passing.

## Milestones
| # | Name | Scope | Dependencies | Status |
|---|------|-------|-------------|--------|
| 1 | Codebase Analysis (Exploration) | Explore codebase (TaskbarWindow.cpp, ClockWidget.cpp, TrayIconScraper.cpp, etc.) to understand current structure and formulate exact implementation strategy. | None | DONE |
| 2 | Feature Implementation (Worker) | Implement display spoofing, tray-clock gap adjustment, 2-row tray layout, tray icons fixing, tray click/double-click actions, and properties panel integrations. | M1 | DONE |
| 3 | Verification & Auditing (Review/Test/Audit) | Run review, test with challengers, and verify code authenticity with Forensic Auditor. | M2 | DONE |
| 4 | Final Verification & Binary Signing | Sign compiled binaries using elite-easysigner and verify everything is working cleanly. | M3 | DONE |

## Interface Contracts
- Registry Path: `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`
  - DWORD: `EnableTwoRowTray` (0: Legacy / Inline, 1: 2-Row tray wrapping with smaller icons)
  - Other existing keys must be preserved.
- UI elements:
  - Taskbar Properties dialog (IDC_TWO_ROW_TRAY or similar checkbox ID in resource.h).
  - Must mirror Settings EXE and standalone CPL.
