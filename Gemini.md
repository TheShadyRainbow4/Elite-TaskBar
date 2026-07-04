# EliteSoftwareTech Co. - EliteTaskbar Development Rules

## 1. Feature Preservation & Settings Toggles
During development, **NEVER** completely remove or obliterate an existing feature or implementation method, even if a superior one is being developed.
Instead, you must:
1. Preserve the old code path.
2. Add a new checkbox/radio toggle in the UI (`TaskbarProperties.cpp` and `resources.rc`).
3. Add a registry key switch to dynamically flip between the legacy behavior and the new behavior at runtime.
4. Explain the limitation and options to the user.

## 2. Documentation and Changelog Auditing
- After *every single* major file edit or code structural change, `CHANGELOG.md` **MUST** be updated to reflect exactly what was changed and why.
- You must frequently review `CHANGELOG.md` to ensure it is in sync with the current build state.
- Do not let documentation rot. If you add a new configuration variable or registry key, you must document it.

## 3. UI/UX Rules
- Flat design is absolutely forbidden.
- Native visual styles (`EnableVisualStyles()`) must always be utilized.
- UI elements must never rely on pure hard-coded colors; they must use system colors or rich 3D gradients.
- Always include an "i" or "Help" mechanism where applicable.
- All interactive controls MUST feature hover tooltips (when possible) or clear descriptive labels.

## 4. Git Hygiene
- Do not push massive binary objects to the repository unless strictly necessary. Ensure things like `.pdf` or compiler temporary objects are stripped or placed in `.gitignore`.
- Backups and outputs should be kept out of remote tracking where applicable, while signed binaries must be preserved.
