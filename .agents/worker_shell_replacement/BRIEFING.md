# BRIEFING — 2026-07-05T07:41:44-07:00

## Mission
Implement Phase XI (Desktop Window, GDI wallpaper rendering, and SysListView32 desktop grid) and Phase XIX (Open-Shell fallback Start Menu integration).

## 🔒 My Identity
- Archetype: implementer-qa-specialist
- Roles: implementer, qa, specialist
- Working directory: C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\worker_shell_replacement\
- Original parent: 74156502-3f15-494c-9ddd-4f02369d5aea
- Milestone: Phase XI and Phase XIX

## 🔒 Key Constraints
- CODE_ONLY network mode.
- Use visual style guidelines (WinForms/Win32 classic, no flat design, etc.).
- Maintain settings toggles for every feature and registry integration (GEMINI.md).
- Keep CHANGELOG.md and README.md updated after edits.
- Use only build.ps1 chain for building.

## Current Parent
- Conversation ID: 74156502-3f15-494c-9ddd-4f02369d5aea
- Updated: not yet

## Task Summary
- **What to build**: Custom desktop window (Progman), background painting with GDI+ for wallpaper styling, and SysListView32 grid showing desktop files via Shell namespace binder. Open-Shell fallback on taskbar Start Button click.
- **Success criteria**: Shell compiles correctly on both x86 and x64 targets; settings UI displays tabs and controls correctly; wallpaper rendering works; desktop icon grid renders items; clicking start button launches Open-Shell if configured.
- **Interface contracts**: C:\Users\Administrator\Desktop\Elite-TaskBar\PROJECT.md
- **Code layout**: SourceFiles directory

## Key Decisions Made
- [TBD]

## Artifact Index
- [TBD]

## Change Tracker
- **Files modified**: None
- **Build status**: Untested
- **Pending issues**: None

## Quality Status
- **Build/test result**: Untested
- **Lint status**: Untested
- **Tests added/modified**: None

## Loaded Skills
- None
