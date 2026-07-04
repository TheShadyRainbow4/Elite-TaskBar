# EliteSoftwareTech Co. - EliteTaskbar Development Rules

## 1. Feature Preservation & Settings Toggles
During development, **NEVER** completely remove or obliterate an existing feature or implementation method, even if a superior one is being developed.
Instead, you must:
1. Preserve the old code path.
2. Add a new checkbox/radio toggle in the UI (`TaskbarProperties.cpp` and `resources.rc`).
3. Add a registry key switch to dynamically flip between the legacy behavior and the new behavior at runtime.
4. Explain the limitation and options to the user.

## 2. Mandatory Pre-Flight & Changelog Rules
**CRITICAL ENFORCEMENT:** You are absolutely FORBIDDEN from starting a task or making ANY code edits before you have completely read and reviewed every `.md` and `.txt` file in the project directory (including this one). 
- **Pre-Flight:** Read all documentation files first.
- **Action:** Only after reading documentation, follow the user's explicit task directives.
- **Post-Flight:** After *every single* file edit, you MUST immediately update `CHANGELOG.md` to reflect exactly what was changed and why. Never skip this step.

## 3. UI/UX Rules
- Flat design is absolutely forbidden.
- Native visual styles (`EnableVisualStyles()`) must always be utilized.
- UI elements must never rely on pure hard-coded colors; they must use system colors or rich 3D gradients.
- Always include an "i" or "Help" mechanism where applicable.
- All interactive controls MUST feature hover tooltips (when possible) or clear descriptive labels.

## 4. Git Hygiene
- Do not push massive binary objects to the repository unless strictly necessary. Ensure things like `.pdf` or compiler temporary objects are stripped or placed in `.gitignore`.
- Backups and outputs should be kept out of remote tracking where applicable, while signed binaries must be preserved.
