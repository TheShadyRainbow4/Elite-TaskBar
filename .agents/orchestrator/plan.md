# Plan - Codebase Exploration and Analysis

This plan addresses the initial exploration of the 7 new requirements:
1. **R1**: Small Icon Tiles View Mode (in Win32Explorer)
2. **R2**: Group by Type Default & Options UI (in Win32Explorer)
3. **R3**: Default Taskbar Mode Fix (in EliteTaskbar)
4. **R4**: Tray Overflow Fix (in EliteTaskbar)
5. **R5**: UWP App Icons Fix (in EliteTaskbar)
6. **R6**: Taskbar Properties UI Glitch (in EliteSettings / TaskbarProperties.cpp)
7. **R7**: High-DPI Text Blurriness Fix (in EliteSoftware suite)

## Work Breakdown

### Step 1: Dispatch 3 Explorer Subagents
We will spawn 3 parallel Explorers:
- **Explorer 1** (`explorer_r1_r4`): Analyze `Win32Explorer` (R1 Small Icon Tiles View and R2 Group by Type default & built-in Options UI).
- **Explorer 2** (`explorer_r2_r5`): Analyze `EliteTaskbar` settings defaults (R3 Default Taskbar Mode) and custom properties UI (R6 Taskbar Properties UI Glitch).
- **Explorer 3** (`explorer_r3_r6`): Analyze `EliteTaskbar` features (R4 Tray Overflow Click/Drag Fix, R5 UWP App Icons Fix, and R7 High-DPI Text Blurriness).

### Step 2: Implementation Milestones
We have structured the implementation into three sequential milestones:
- **Milestone 1**: Win32Explorer view modes & Options UI (R1 & R2) - Currently executing under Worker 1.
- **Milestone 2**: EliteTaskbar settings defaults & UI fixes (R3, R6, R8, R9) - Focuses on default Independent mode (R3), properties window hover rendering fix (R6), About Dialog positioning/credits (R8), and CPL Apply button hang + restart crash (R9).
- **Milestone 3**: EliteTaskbar tray overflow, UWP icons, and high-DPI scaling (R4, R5, R7) - Focuses on tray overflow clickable toolbar subclassing (R4), shell-based UWP app icon retrieval (R5), and DPI scaling font / geometry adjustments (R7).

### Step 3: Verification Gates
Each milestone will be verified using static review, empirical testing, and forensic audit before proceeding to the next.
