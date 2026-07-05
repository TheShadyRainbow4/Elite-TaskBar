# E2E Test Infra: EliteTaskbar

## Test Philosophy
- Opaque-box, requirement-driven. No dependency on implementation design.
- Methodology: Category-Partition + BVA + Pairwise + Workload Testing.

## Feature Inventory
| # | Feature | Source (requirement) | Tier 1 | Tier 2 | Tier 3 |
|---|---------|---------------------|:------:|:------:|:------:|
| 1 | Multi-Monitor Flyouts | ORIGINAL_REQUEST R1.1 | 5 | 5 | ✓ |
| 2 | Clock/Tray Gap & 2-Row Tray | ORIGINAL_REQUEST R1.2 | 5 | 5 | ✓ |
| 3 | Tray Icon Backgrounds & Fallbacks | ORIGINAL_REQUEST R1.3 | 5 | 5 | ✓ |
| 4 | Settings UI & About dialog | ORIGINAL_REQUEST R2.1, R2.2 | 5 | 5 | ✓ |
| 5 | Clean up old.exe & reload bug | ORIGINAL_REQUEST R2.3, R2.4 | 5 | 5 | ✓ |
| 6 | Progman Multi-Display & Desktop Tab | ORIGINAL_REQUEST R3.1, R3.2 | 5 | 5 | ✓ |
| 7 | Keyboard Hooks (Win+R) | ORIGINAL_REQUEST R4.1 | 5 | 5 | ✓ |
| 8 | Tray Click Actions | ORIGINAL_REQUEST R5.1 | 5 | 5 | ✓ |
| 9 | Taskbar Extras (Seconds, QuickLaunch) | ORIGINAL_REQUEST R5.2 | 5 | 5 | ✓ |
| 10| Win32Explorer view modes | ORIGINAL_REQUEST R5.3 | 5 | 5 | ✓ |

## Test Architecture
- Test runner: automated PowerShell scripts verifying registry changes, process lifecycle, and output files.
- Test case format: Registry values validation, dynamic window queries, folder contents, and interactive UI verification.
- Directory layout: tests will be located under `Subagent_Tests/` and output validation will be managed by `verify_final_polish.ps1`.

## Real-World Application Scenarios (Tier 4)
| # | Scenario | Features Exercised | Complexity |
|---|----------|--------------------|------------|
| 1 | Full Desktop Shell Replacement | Progman multi-display, Win+R hooks, Desktop tab slideshow, 2-Row tray, Quick Launch. | High |
| 2 | Multi-Monitor Workspace Setup | Flyout spoofing on secondary monitor, clock seconds, taskbar tray actions. | Medium |
| 3 | Shell Upgrade & Migration Cleanup | Old.exe file deletion, Settings Apply reload without duplicate windows. | Medium |

## Coverage Thresholds
- Tier 1: ≥5 test cases per feature (happy-path testing)
- Tier 2: ≥5 boundary / corner cases per feature (missing registry values, multi-monitor configuration limits)
- Tier 3: Pairwise combination of major feature interactions
- Tier 4: At least 3 real-world application scenarios
