# Plan - Milestone 2 Remediation & Custom Controls Polish

This plan addresses the five remaining issues identified in the Milestone 2 verification gate:
1. Fix PowerShell build script error handling in `build_settings.ps1` under `$ErrorActionPreference = 'Stop'`.
2. Implement hover tooltips for all interactive controls inside `TaskbarProperties.cpp`.
3. Change typography from "MS Shell Dlg" to "Segoe UI" in `resources.rc`.
4. Implement custom native C++ Help Dialog and About Dialog (with collapse/expand) and integrate them.
5. Correct target executable name in `build_sign.ps1` to use signed x64/x86 Elite-EasySigner.

## Work Breakdown

### Step 1: Dispatch Worker
Spawn a worker to modify:
- `build_settings.ps1` (error-handling wrap)
- `SourceFiles/resources.rc` (Segoe UI typography, dialog templates for Help and About)
- `SourceFiles/resource.h` (add IDs for Help/About controls)
- `SourceFiles/TaskbarProperties.cpp` (implement tooltips, Help/About dialog procedures, menu integration)
- `build_sign.ps1` (target correct Elite-EasySigner)

### Step 2: Verification
Spawn verification subagents (Reviewers, Challengers, Auditor) to audit and run tests.

### Step 3: Synthesis & Reporting
Compile findings and deliver to parent.
