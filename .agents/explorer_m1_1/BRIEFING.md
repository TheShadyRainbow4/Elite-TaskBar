# BRIEFING — 2026-07-05T01:08:55-07:00

## Mission
Investigate the EliteTaskbar codebase to identify the best way to initialize and manage a borderless, bottom-Z-order window class named "Progman", define its lifecycle, suggest class registration/creation, and provide architectural recommendations.

## 🔒 My Identity
- Archetype: explorer
- Roles: Read-only investigation, analysis, synthesis, structured reporting
- Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_1\
- Original parent: 42744ab6-8967-4ff7-aded-5eb3f035bb39
- Milestone: progman-window-architecture

## 🔒 Key Constraints
- Read-only investigation — do NOT implement or modify any code.
- No dark mode, strictly legacy/classic Win32 layouts/WinForms rules apply if UI changes are proposed.
- Build and test commands if code is changed (not applicable since read-only).
- Maintain all existing features, no removing of features.

## Current Parent
- Conversation ID: 42744ab6-8967-4ff7-aded-5eb3f035bb39
- Updated: 2026-07-05T01:08:55-07:00

## Investigation State
- **Explored paths**:
  - `SourceFiles/main.cpp` — Examined WinMain entry point, exception handler, CLI arguments, and mutex handling.
  - `SourceFiles/TaskbarWindow.cpp` — Investigated class registrations, window creation, screen display enumeration, message loop, and cleanup routines.
  - `SourceFiles/Config.h` — Checked configuration parameters and registry helper functions.
  - `.agents/orchestrator_shell_replacement/plan.md` — Verified milestone requirements for Phase XI.
- **Key findings**:
  - EliteTaskbar acts as a shell replacement by hiding `Shell_TrayWnd` and running an STA message pump in `main.cpp`.
  - There is currently no `Progman` window class registered or managed in the codebase.
  - In `Replace` mode, registering and creating the custom `Progman` class is required to replace the native desktop and display custom wallpaper/icons.
  - A bottom-Z-order window requires handling `WM_MOUSEACTIVATE` (returning `MA_NOACTIVATE`) and `WM_WINDOWPOSCHANGING` (setting `hwndInsertAfter = HWND_BOTTOM`) to prevent it from rising above other windows.
- **Unexplored areas**:
  - Wallpaper GDI rendering (delegated to `explorer_m1_2`).
  - Desktop Icon Grid and Shell Folder binding (delegated to `explorer_m1_3`).

## Key Decisions Made
- Recommend creating a dedicated module (`DesktopWindow.h` and `DesktopWindow.cpp`) rather than bloating `TaskbarWindow.cpp`.
- Recommend checking if a `Progman` window already exists and hiding/restoring it in `Replace` mode to allow peaceful coexistence.

## Artifact Index
- C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_1\handoff.md — Investigation and architectural recommendation report.
