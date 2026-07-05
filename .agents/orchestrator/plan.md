# Plan - Milestone 3: EliteTaskbar Advanced Features & View Modes Integration

This plan coordinates the implementation and verification of the remaining advanced features (R1 Small Icon Tiles View Mode, R2 Group by Type, R3 Default Taskbar Mode, R4 Tray Overflow Fix, R5 UWP App Icons, and R7 High-DPI Text Blurriness Fix, plus R8 About Dialog Spacing and R9 Apply Button Hang) using a parallel swarm of 7 subagents.

## Swarm Roster (7 Agents)
1. **Explorer 4** (`teamwork_preview_explorer`): Investigate and reconcile file drift between `SourceFiles/` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/`, and analyze the implementation gaps for R4, R5, R7, R8, R9.
2. **Worker 4** (`teamwork_preview_worker`): Implement the code updates, sync the two TaskbarWindow.cpp copies, ensure warnings-as-errors compliance, and run `build.ps1`.
3. **Reviewer 5** (`teamwork_preview_reviewer`): Code correctness and structural review.
4. **Reviewer 6** (`teamwork_preview_reviewer`): UI/UX compliance review (tooltips, Segoe UI fonts, Help/About dialog styling).
5. **Challenger 3** (`teamwork_preview_challenger`): Empirical tests on tray overflow mouse events, UWP icon retrieval, and high-DPI scaling rendering.
6. **Challenger 4** (`teamwork_preview_challenger`): Empirical tests on Win32Explorer view modes (Small Icon Tiles, Group by Type Options UI, Options saving).
7. **Auditor 3** (`teamwork_preview_auditor`): Forensic integrity verification of all codebases.

## Execution Sequence

### Phase 1: Exploration
- **Dispatch**: Spawn Explorer 4.
- **Task**: Locate and document all differences between the Taskbar source file copies. Provide a precise remediation/sync path and implementation guidelines for the missing requirements.

### Phase 2: Implementation & Sync
- **Dispatch**: Spawn Worker 4.
- **Task**:
  - Unify `TaskbarWindow.cpp`, `TrayIconScraper.cpp`, `TaskbarProperties.cpp`, `resources.rc`, `resource.h` between `SourceFiles/` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/`.
  - Fix any missing tooltip elements in TaskbarProperties.cpp or settings dialogs.
  - Implement dynamic DPI scaling and `WM_DPICHANGED` for GDI rendering in `TaskbarWindow.cpp`.
  - Compile the entire solution via `C:\Users\Administrator\Desktop\Elite-TaskBar\build.ps1`.

### Phase 3: Swarm Verification
- **Dispatch**: Spawn Reviewers (5 & 6), Challengers (3 & 4), and Auditor 3.
- **Task**: Review code quality, verify GUI styling requirements, empirically test all targets, and audit development integrity.

### Phase 4: Sign-off
- Verify all pass criteria are met. Update project status and report to Sentinel.
