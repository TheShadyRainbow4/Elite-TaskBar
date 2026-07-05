# Original User Request

## 2026-07-05T23:31:24Z

You are the Milestone 5 Sub-Orchestrator for the Elite-Taskbar project.
Your working directory is C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\sub_orch_m5.
Your parent is 524a5ab5-f0bb-494c-a5f6-b5aa14c7e677 (current conversation).

Your task is to implement the following features:
1. Fix the Start Menu and Volume/Sound flyouts appearing on the wrong screen (or wrong side) when invoked from a secondary taskbar. Implement dynamic primary display spoofing to trick the invoked UI into rendering on the correct display.
2. Fix the large gap between the clock and tray area.
3. Ensure tray items pull correct icons (fix blank icons), remove white backgrounds above tray items, and add an option to display tray items in two rows by wrapping them within the standard taskbar height (using smaller icons).
4. Implement tray actions:
   - Win32Explorer Tray: Use correct Win32Explorer icon. Single-click = About window, Double-click = New window.
   - Taskbar Tray: Single-click = About window, Double-click = Settings CPL.
   - Desktop Replacement Tray: Create a dedicated tray item using the native desktop tree icon. Double-click = Show/hide desktop icons. Context menu = Toggle desktop on/off, Restart.

Execution Plan:
1. Initialize your BRIEFING.md and progress.md under C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\sub_orch_m5.
2. Formulate your scope document `SCOPE.md` describing your implementation plan.
3. Run the Explorer-Worker-Reviewer loop. You must spawn:
   - 3 Explorer agents to analyze the codebase (e.g. TaskbarWindow.cpp, ClockWidget.cpp, TrayIconScraper.cpp, etc.) and recommend strategies for these features.
   - 1 Worker agent to implement the changes and build/test using `.\build.ps1`.
   - 2 Reviewer agents to verify correctness, visual style layout, and warnings.
   - 2 Challenger agents to run tests.
   - 1 Forensic Auditor agent to verify code authenticity and check for cheating.
4. Integrate settings toggles (e.g. 2-Row Tray mode) in `TaskbarProperties.cpp` and `resources.rc`. Ensure CPL and Settings EXE are perfectly mirrored.
5. All compiled binaries must be signed using `elite-easysigner`.
6. When complete, update SCOPE.md and progress.md, write handoff.md, and send a completion message to your parent.

Mandatory rules:
- NEVER write, modify, or create source code files directly.
- NEVER run build/test commands yourself.
- Follow GEMINI.md rules: no flat design, etc.
- Include the integrity warning in the worker prompt.
- Perform the liveness check (heartbeat) every 10 minutes.
