# Handoff Report: R3 Default Taskbar Mode Fix & R6 Taskbar Properties UI Glitch

## 1. Observation

### R3: Default Taskbar Mode Fix
In `SourceFiles\main.cpp` (and the copy `Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\TaskbarMain.cpp`), `g_Config.Mode` is initialized to `TaskbarMode::Independent` by default:
```cpp
// main.cpp line 17
EliteTaskbarConfig g_Config = { L"", TaskbarMode::Independent, ButtonWidthMode::Auto, TrayOverflowMode::Win7Flyout, false, false, {} };
```
In `SourceFiles\TaskbarProperties.cpp` (and the copy `Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\TaskbarProperties.cpp`), the registry query for `TaskbarMode` is:
```cpp
// TaskbarProperties.cpp line 452
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0;
            DWORD cbData = sizeof(DWORD);
            
            if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
            }
            ...
            RegCloseKey(hKey);
        }
```
If the registry key does not exist at all, the dialog initialization skips this entire block and leaves all radio buttons in the group unchecked.

In `SourceFiles\EliteSettings.ps1` (and the copy `Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\EliteSettings.ps1`), the loading of `TaskbarMode` registry setting checks `$val` against `0` and `1`:
```powershell
# EliteSettings.ps1 line 420
        $val = (Get-ItemProperty -Path $global:regPathElite -Name "TaskbarMode" -ErrorAction SilentlyContinue).TaskbarMode
        if ($val -eq 0) { $rdo_ModeIndep.Checked = $true }
        elseif ($val -eq 1) { $rdo_ModeReplace.Checked = $true }
        else { $rdo_ModeSecond.Checked = $true }
```
When `TaskbarMode` is missing (first-run/clean registry), `$val` is `$null`. Since `$null -eq 0` is false and `$null -eq 1` is false, it falls through to the `else` block checking `$rdo_ModeSecond` (Secondary Only) instead of Independent mode.

---

### R6: Taskbar Properties UI Glitch
All custom property sheet page dialog procedures in `SourceFiles\TaskbarProperties.cpp` contain identical handlers for static and button controls background color messages:
```cpp
// TaskbarProperties.cpp line 433 (TaskbarSettingsDlgProc)
// Similar blocks exist at line 529 (NativeSettingsDlgProc), 779 (MultiMonSettingsDlgProc), 1007 (ToolbarsSettingsDlgProc), 1040 (GenericPageDlgProc)
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORBTN: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndControl = (HWND)lParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        if (IsThemeActive()) {
            DrawThemeParentBackground(hwndControl, hdcStatic, NULL);
            return (INT_PTR)GetStockObject(NULL_BRUSH);
        }
        return (INT_PTR)GetSysColorBrush(COLOR_BTNFACE);
    }
```
`DynScrollAreaProc` (the window procedure for the custom scrolling area used inside Multi-Monitor and Start Menu properties) contains a similar block:
```cpp
// TaskbarProperties.cpp line 697
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndControl = (HWND)lParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            if (IsThemeActive()) {
                DrawThemeParentBackground(hwndControl, hdcStatic, NULL);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
```

---

## 2. Logic Chain

### R3: Default Taskbar Mode Fix
1. By default, `main.cpp` initializes the memory config state to `TaskbarMode::Independent`.
2. However, if the registry key `Software\EliteSoftware\Win32Explorer\Advanced` does not exist, the C++ properties dialog does not check any radio button, leaving the group blank.
3. In the PowerShell settings GUI (`EliteSettings.ps1`), when the key is missing, `$val` evaluates to `$null`, falling through to the `else` block and checking the "Secondary Only" radio button. When the user saves settings, "Secondary Only" (value 2) or "Replace" is committed.
4. To fix this, the default state of both C++ and PowerShell properties GUI should explicitly select "Independent" (value 0) when the registry key or value is missing.

### R6: Taskbar Properties UI Glitch
1. In `WM_INITDIALOG`, each dialog page calls `EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB)`.
2. When themed dialog textures are enabled on common controls v6 (enabled in `app.manifest`), the theme manager automatically handles text and control backgrounds for check boxes, radio buttons, and static text controls to blend transparently on the tab textured background.
3. The custom `WM_CTLCOLORSTATIC` and `WM_CTLCOLORBTN` handlers override this native behavior by calling `DrawThemeParentBackground` and returning `NULL_BRUSH`.
4. During initial painting of the property page, child controls are drawn before the parent dialog has completed painting its background texture. As a result, `DrawThemeParentBackground` copies uninitialized dialog pixels (making controls invisible or solid black).
5. Only when the mouse hovers over a control is that control individually repainted, copying the fully-painted parent background texture and making it visible.
6. Removing the redundant and broken `WM_CTLCOLORSTATIC`/`WM_CTLCOLORBTN` handlers from all dialog procedures resolves the invisibility lag, allowing native theme painting to handle background transparency seamlessly.
7. For the custom window class `DynScrollArea` (which is not a dialog page and does not use `EnableThemeDialogTexture`), removing `DrawThemeParentBackground` from `WM_CTLCOLORSTATIC`/`WM_CTLCOLORBTN` and returning the transparent `NULL_BRUSH` with `SetBkMode(hdcStatic, TRANSPARENT)` lets GDI render the text transparently over the already-drawn parent background without rendering glitchiness.

---

## 3. Caveats
- **Win32Explorer Copies**: The project maintains copies of `EliteSettings.ps1`, `TaskbarProperties.cpp`, and `main.cpp` (as `TaskbarMain.cpp`) under `Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\`. These files must be updated identically to prevent inconsistencies.
- **Classic Theme Fallback**: If the OS has visual styles completely disabled (Windows Classic style), removing the `WM_CTLCOLORSTATIC` and `WM_CTLCOLORBTN` handlers is safe because the default dialog procedure natively returns the correct solid gray brush (`COLOR_3DFACE` / `COLOR_BTNFACE`).

---

## 4. Conclusion

The default taskbar mode must be corrected by ensuring that the C++ properties dialog checks `IDC_MODE_INDEPENDENT` by default, and the PowerShell settings applet defaults to `Independent` if the registry value is missing.
The properties UI rendering glitch is caused by calling `DrawThemeParentBackground` inside custom `WM_CTLCOLOR` handlers, which conflicts with native theme texturing. Removing these handlers from the dialog procedures and simplifying the handler in the scroll container resolves the bug.

### Proposed Code Changes:

#### 1. Taskbar Mode Default Fix (C++ Properties Dlg)
In `SourceFiles\TaskbarProperties.cpp` and `Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\TaskbarProperties.cpp` inside `TaskbarSettingsDlgProc` (around line 444):
```cpp
// BEFORE
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        ...
        HKEY hKey;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0;
            DWORD cbData = sizeof(DWORD);
            
            if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
            }
```

```cpp
// AFTER
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        ...
        // Check Independent by default first
        SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
        
        HKEY hKey;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0;
            DWORD cbData = sizeof(DWORD);
            
            if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                // Reset checked state according to registry value
                SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_UNCHECKED, 0);
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
            }
```

#### 2. Taskbar Mode Default Fix (PowerShell Settings GUI)
In `SourceFiles\EliteSettings.ps1` and `Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\EliteSettings.ps1` (around line 420):
```powershell
# BEFORE
        $val = (Get-ItemProperty -Path $global:regPathElite -Name "TaskbarMode" -ErrorAction SilentlyContinue).TaskbarMode
        if ($val -eq 0) { $rdo_ModeIndep.Checked = $true }
        elseif ($val -eq 1) { $rdo_ModeReplace.Checked = $true }
        else { $rdo_ModeSecond.Checked = $true }
```

```powershell
# AFTER
        $val = (Get-ItemProperty -Path $global:regPathElite -Name "TaskbarMode" -ErrorAction SilentlyContinue).TaskbarMode
        if ($val -eq 1) { $rdo_ModeReplace.Checked = $true }
        elseif ($val -eq 2) { $rdo_ModeSecond.Checked = $true }
        else { $rdo_ModeIndep.Checked = $true } # Defaults to Independent (0 or null)
```

#### 3. Properties Window Drawing Fix (C++ Properties Dlg)
In `SourceFiles\TaskbarProperties.cpp` and `Win32Explorer_26.0.3.0\App_Source\EliteTaskbar\TaskbarProperties.cpp`:
- **Delete** the `WM_CTLCOLORSTATIC` and `WM_CTLCOLORBTN` cases entirely from the following procedures:
  - `TaskbarSettingsDlgProc` (lines 433-443)
  - `NativeSettingsDlgProc` (lines 529-539)
  - `MultiMonSettingsDlgProc` (lines 779-789)
  - `ToolbarsSettingsDlgProc` (lines 1007-1017)
  - `GenericPageDlgProc` (lines 1040-1050)
  
- **Modify** `DynScrollAreaProc` (around line 697) to remove `DrawThemeParentBackground` call:
```cpp
// BEFORE
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC hdcStatic = (HDC)wParam;
            HWND hwndControl = (HWND)lParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            if (IsThemeActive()) {
                DrawThemeParentBackground(hwndControl, hdcStatic, NULL);
                return (LRESULT)GetStockObject(NULL_BRUSH);
            }
            return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
        }
```

```cpp
// AFTER
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORBTN: {
            HDC hdcStatic = (HDC)wParam;
            SetBkMode(hdcStatic, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }
```

---

## 5. Verification Method

### Build Verification
1. Implement the changes in `SourceFiles\TaskbarProperties.cpp`, `SourceFiles\EliteSettings.ps1`, and their duplicates in `Win32Explorer_26.0.3.0`.
2. Compile the project using the orchestrator build script:
   `pwsh build.ps1`
3. Ensure no compiler or resource compiler warnings/errors are emitted.

### Behavioral Verification
1. **R3 Default Mode Fix**:
   - Clear existing registry entries under `HKCU\Software\EliteSoftware\Win32Explorer\Advanced` using `reg delete`.
   - Run `EliteTaskbar.exe`.
   - Open properties. Confirm the "Independent" mode radio button is checked.
   - Confirm that the native Windows taskbar is not hidden/replaced by default.
2. **R6 Properties UI Glitch Fix**:
   - Run the settings property sheet: `EliteTaskbar.exe /settings`.
   - Toggle through each tab ("Taskbar", "Native Settings", "Multi-Monitor", "Start Menu", "Toolbars").
   - Confirm that all group boxes, static labels, checkboxes, and radio buttons are immediately visible and drawn correctly on page load, without requiring a mouse hover.
