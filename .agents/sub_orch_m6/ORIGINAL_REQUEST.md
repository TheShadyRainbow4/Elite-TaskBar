# Original User Request

## Initial Request — 2026-07-05T17:11:14-07:00

You are the Milestone 6 Sub-Orchestrator for the Elite-Taskbar project.
Your working directory is C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\sub_orch_m6.
Your parent is 524a5ab5-f0bb-494c-a5f6-b5aa14c7e677 (current conversation).

Your task is to implement the following features:
1. Fix the Start Menu settings tab (currently empty / hover-to-reveal bug). Ensure the controls are shown correctly without requiring mouse hover.
2. Fix the About dialog so hide/close buttons (such as "Less Info <<" and "Okay" or "Close") are properly visible and positioned when expanded, matching spacing requirements.
3. Fix the bug where reloading after applying settings opens several Win32Explorer windows.
4. Automatically clean up and delete all `*old*.exe` or `*Old*.exe` files (e.g. `EliteEverything_old*.exe`, `EliteSettings_old*.exe` in the workspace root and build output folders) after creation/migration.

Execution Plan:
1. Initialize your BRIEFING.md and progress.md under C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\sub_orch_m6.
2. Formulate your scope document `SCOPE.md` describing your implementation plan.
3. Run the Explorer-Worker-Reviewer loop. You must spawn:
   - 3 Explorer agents to analyze the codebase (e.g. TaskbarProperties.cpp, resources.rc, build.ps1, etc.) and recommend strategies for these features.
   - 1 Worker agent to implement the changes and build/test using `.\build.ps1`.
   - 2 Reviewer agents to verify correctness, visual style layout, and warnings.
   - 2 Challenger agents to run tests.
   - 1 Forensic Auditor agent to verify code authenticity and check for cheating.
4. Ensure CPL and Settings EXE are perfectly mirrored.
5. All compiled binaries must be signed using `elite-easysigner`.
6. When complete, update SCOPE.md and progress.md, write handoff.md, and send a completion message to your parent.

Mandatory rules:
- NEVER write, modify, or create source code files directly.
- NEVER run build/test commands yourself.
- Follow GEMINI.md rules: no flat design, etc.
- Include the integrity warning in the worker prompt.
- Perform the liveness check (heartbeat) every 10 minutes.
