# BRIEFING — 2026-07-05T08:08:11Z

## Mission
Investigate desktop icon grid creation (Progman, SHELLDLL_DefView, SysListView32), binding SysListView32 to desktop folders using IShellFolder, and SHChangeNotifyRegister dynamic refresh.

## 🔒 My Identity
- Archetype: Explorer
- Roles: Read-only investigator, analytical synthesizer
- Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_3
- Original parent: 42744ab6-8967-4ff7-aded-5eb3f035bb39
- Milestone: Milestone 1, Task 3 (Desktop Icon Grid Investigation)

## 🔒 Key Constraints
- Read-only investigation — do NOT implement
- CODE_ONLY network mode: No external network access

## Current Parent
- Conversation ID: 42744ab6-8967-4ff7-aded-5eb3f035bb39
- Updated: 2026-07-05T08:08:11Z

## Investigation State
- **Explored paths**:
  - `ReactOS-Source-Shell/explorer/desktop.cpp` — Analysed desktop thread initialization and interface binding.
  - `ReactOS-Source-Shell/explorer/rshell.cpp` — Investigated ordinals for desktop creation (SHCreateDesktop Ordinal 200).
  - Shell API structures for list-view binding and dynamic changes.
- **Key findings**:
  - Main desktop container must mimic the class name hierarchy of native Windows (`Progman` -> `SHELLDLL_DefView` -> `SysListView32`) for 100% compatibility with third-party desktop tools.
  - `IShellFolder` obtained from `SHGetDesktopFolder` automatically aggregates physical user desktop directories (`CSIDL_DESKTOPDIRECTORY`) and public desktop directories (`CSIDL_COMMON_DESKTOPDIRECTORY`), as well as virtual items.
  - Shell notifications can be registered using `SHChangeNotifyRegister` on the virtual desktop PIDL. Setting `SHCNRF_NewDelivery` enables message delivery via `WM_COPYDATA` or direct parameters with the file system change event and target PIDLs.
- **Unexplored areas**:
  - Actual implementation of wallpaper drawing in `Progman` (deferred to explorer_m1_2).

## Key Decisions Made
- Use debounced list-view refresh mechanism for shell change events (rather than inline incremental modifications) to simplify synchronization and avoid performance lags.
- Leverage `ShellExecuteExW` with `SEE_MASK_IDLIST` and PIDLs for double-click launch.

## Artifact Index
- C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_3\ORIGINAL_REQUEST.md — Original task description
- C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_3\progress.md — Task heartbeat
- C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\explorer_m1_3\handoff.md — Full findings and recommendation report

