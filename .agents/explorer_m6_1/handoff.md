# Handoff Report — Start Menu Tab Fix Strategy

## 1. Observation
- **`CreateDynScrollArea` Implementation**:
  In `SourceFiles/TaskbarProperties.cpp` (lines 921-929):
  ```cpp
  HWND CreateDynScrollArea(HWND hwndDlg, int idc_placeholder) {
      InitDynScrollClass();
      HWND hPlaceholder = GetDlgItem(hwndDlg, idc_placeholder);
      RECT rc;
      GetClientRect(hwndDlg, &rc);
      HWND hScroll = CreateWindowExW(WS_EX_CONTROLPARENT, L"EliteDynScrollArea", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, 0, rc.right, rc.bottom, hwndDlg, NULL, g_hInstance, NULL);
      if (hPlaceholder) DestroyWindow(hPlaceholder);
      return hScroll;
  }
  ```
  The same code exists in the submodule path `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp` (lines 912-920).
- **`IDD_STARTMENU_PROPS` Template**:
  In `SourceFiles/resources.rc` (lines 77-85):
  ```rc
  IDD_STARTMENU_PROPS DIALOGEX 0, 0, 252, 218
  STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
  CAPTION "Start Menu"
  FONT 8, "Segoe UI Semibold", 600, 0, 0x1
  BEGIN
      LTEXT           "Configure start menu per-monitor:", IDC_STATIC, 7, 7, 238, 8
      CONTROL         "", IDC_DYN_SCROLLAREA, "Static", SS_BLACKFRAME | NOT WS_VISIBLE, 7, 20, 238, 170
      CONTROL         "Use Fallback Start Menu (Open-Shell Integration)", IDC_FALLBACK_STARTMENU_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 7, 195, 238, 10
  END
  ```
  The same template exists in the submodule copy `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc` (lines 76-84).
- **`StartMenuSettingsDlgProc`**:
  Currently, `StartMenuSettingsDlgProc` (lines 1191-1233 in `SourceFiles/TaskbarProperties.cpp`) only initializes and updates the `IDC_FALLBACK_STARTMENU_ENABLED` checkbox. It calls `CreateDynScrollArea` but does not add any child controls or monitor-specific items inside the scroll area.
- **`MultiMonSettingsDlgProc`**:
  `MultiMonSettingsDlgProc` (lines 950-1082 in `SourceFiles/TaskbarProperties.cpp`) contains code to dynamically populate the scroll area with System Tray, Clock, and Task Buttons checkboxes, as well as the Start Menu Mode, Start Menu Trigger, Start Orb Theme, and Orb Preview for each monitor.
- **Draft Refactoring (`TaskbarProperties.new.cpp`)**:
  In `SourceFiles/TaskbarProperties.new.cpp`, the per-monitor Start Menu configuration has been refactored into `StartMenuSettingsDlgProc` (lines 386-505) and removed from `MultiMonSettingsDlgProc` (lines 272-360), but this file is not compiled.

## 2. Logic Chain
- **Hover-to-Reveal Bug**:
  1. `CreateDynScrollArea` retrieves the client rectangle of the entire dialog page using `GetClientRect(hwndDlg, &rc)`.
  2. It then calls `CreateWindowExW` with coordinate parameters `0, 0, rc.right, rc.bottom`.
  3. Consequently, the created `EliteDynScrollArea` window (which acts as a scroll container) fills the entire page client area (`252 x 218` DUs).
  4. This window overlaps the static text at the top and the checkbox `IDC_FALLBACK_STARTMENU_ENABLED` at the bottom (y=195, h=10).
  5. Because the scroll container window is higher in the Z-order, it paints over the checkbox. Hovering the mouse over the checkbox triggers a cursor message and forces a repaint of the checkbox, making it temporarily visible.
- **Empty Start Menu Tab**:
  1. The dynamic scroll area is created, but no child controls (groupboxes, comboboxes, preview bitmaps) are populated inside it during `WM_INITDIALOG`.
  2. The actual per-monitor configuration controls currently live under the "Multi-Monitor Components" tab dialog procedure (`MultiMonSettingsDlgProc`).
- **Layout Parity & Parity Requirement**:
  1. To resolve both the empty tab layout and maintain the legacy behavior (as mandated by **GEMINI.md Rule 1**), a runtime layout migration switch is needed.
  2. A new checkbox `IDC_MIGRATE_START_MENU_SETTINGS` is added to the Start Menu tab, backed by the registry key `MigrateStartMenuSettings`.
  3. When `MigrateStartMenuSettings = 1` (default), the per-monitor Start Menu configuration is drawn in the "Start Menu" tab scroll container, and the "Multi-Monitor" tab displays only the compact taskbar checkboxes.
  4. When `MigrateStartMenuSettings = 0`, the legacy layout is preserved: Start Menu per-monitor configuration is drawn in the "Multi-Monitor" tab, and the Start Menu tab's scroll area remains empty.

## 3. Caveats
- This investigation assumes that the Open-Shell integration executables (`StartMenu.exe`) are placed correctly relative to the EliteTaskbar runtime path or fallback paths.
- Registry writes assume standard access rights to `HKCU\Software\EliteSoftware\Win32Explorer\Advanced`.

## 4. Conclusion
The "hover-to-reveal" layout glitch is caused by the scroll container stretching across the entire dialog window due to `GetClientRect(hwndDlg, &rc)` in `CreateDynScrollArea`. The "empty tab" issue is because the per-monitor Start Menu settings have not been integrated into `StartMenuSettingsDlgProc` in the active codebase. 

A unified layout fix and migration toggle must be implemented in both `TaskbarProperties.cpp` / `resources.rc` and their submodule equivalents in `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/`.

## 5. Proposed Code Changes

### A. Resource IDs (`SourceFiles/resource.h` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resource.h`)
Add the following macro definitions:
```cpp
#define IDC_MIGRATE_START_MENU_SETTINGS 295
```

### B. Dialog Templates (`SourceFiles/resources.rc` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resources.rc`)
Update `IDD_STARTMENU_PROPS` to add the new checkbox:
```rc
IDD_STARTMENU_PROPS DIALOGEX 0, 0, 252, 218
STYLE DS_SETFONT | WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
CAPTION "Start Menu"
FONT 8, "Segoe UI Semibold", 600, 0, 0x1
BEGIN
    LTEXT           "Configure start menu per-monitor:", IDC_STATIC, 7, 7, 238, 8
    CONTROL         "", IDC_DYN_SCROLLAREA, "Static", SS_BLACKFRAME | NOT WS_VISIBLE, 7, 20, 238, 170
    CONTROL         "Use Fallback Start Menu (Open-Shell Integration)", IDC_FALLBACK_STARTMENU_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 7, 193, 238, 10
    CONTROL         "Migrate per-monitor settings from Multi-Monitor tab", IDC_MIGRATE_START_MENU_SETTINGS, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 7, 205, 238, 10
END
```

### C. CreateDynScrollArea Fix (`SourceFiles/TaskbarProperties.cpp` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`)
Modify the bounds computation to use the placeholder's screen-to-client mapped rectangle:
```cpp
HWND CreateDynScrollArea(HWND hwndDlg, int idc_placeholder) {
    InitDynScrollClass();
    HWND hPlaceholder = GetDlgItem(hwndDlg, idc_placeholder);
    RECT rc = { 0 };
    if (hPlaceholder) {
        GetWindowRect(hPlaceholder, &rc);
        MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
    } else {
        GetClientRect(hwndDlg, &rc);
    }
    HWND hScroll = CreateWindowExW(WS_EX_CONTROLPARENT, L"EliteDynScrollArea", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hwndDlg, NULL, g_hInstance, NULL);
    if (hPlaceholder) DestroyWindow(hPlaceholder);
    return hScroll;
}
```

### D. Multi-Monitor Dialog Proc (`SourceFiles/TaskbarProperties.cpp` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`)
Expose the migration layout split in `MultiMonSettingsDlgProc`:
- **Under `WM_INITDIALOG`**:
  ```cpp
  hScroll = CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
  if (g_Monitors.empty()) EnumDisplayMonitors(NULL, NULL, TaskbarPropsMonitorEnumProc, 0);
  
  HKEY hKey;
  DWORD migrateVal = 1; // Default to 1
  DWORD cbData = sizeof(DWORD);
  if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
      RegQueryValueExW(hKey, L"MigrateStartMenuSettings", NULL, NULL, (LPBYTE)&migrateVal, &cbData);
      RegCloseKey(hKey);
  }

  int y = 5;
  HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
  RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);

  for (const auto& mon : g_Monitors) {
      WCHAR title[64];
      if (migrateVal) {
          // New Compact Layout (Only taskbar items)
          wsprintfW(title, L"Monitor %d Taskbar", mon.index);
          HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, 60, hScroll, NULL, g_hInstance, NULL);
          SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
          
          HWND hChk1 = CreateWindowExW(0, L"Button", L"System Tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TRAY + mon.index), g_hInstance, NULL);
          HWND hChk2 = CreateWindowExW(0, L"Button", L"Clock", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 120, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_CLOCK + mon.index), g_hInstance, NULL);
          HWND hChk3 = CreateWindowExW(0, L"Button", L"Task Buttons", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 40, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TBTN + mon.index), g_hInstance, NULL);
          
          SendMessageW(hChk1, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hChk2, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hChk3, WM_SETFONT, (WPARAM)hFont, 0);
          
          AddTooltip(hScroll, hChk1, L"Show system tray icons on this monitor.");
          AddTooltip(hScroll, hChk2, L"Show clock widget on this monitor.");
          AddTooltip(hScroll, hChk3, L"Show application task buttons on this monitor.");

          if (hKey) {
              DWORD dwValue = 0, cbDataVal = sizeof(DWORD);
              WCHAR val[64];
              wsprintfW(val, L"EnableTray_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbDataVal) == ERROR_SUCCESS && dwValue) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
              else if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
              
              cbDataVal = sizeof(DWORD);
              wsprintfW(val, L"EnableClock_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbDataVal) == ERROR_SUCCESS && dwValue) SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
              else SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
              
              cbDataVal = sizeof(DWORD);
              wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbDataVal) == ERROR_SUCCESS && dwValue) SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
              else SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
          } else {
              if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
              SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
              SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
          }
          y += 70;
      } else {
          // Legacy Full Layout (Both taskbar and start menu items)
          wsprintfW(title, L"Monitor %d (%dx%d)", mon.index, mon.rect.right - mon.rect.left, mon.rect.bottom - mon.rect.top);
          HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, 175, hScroll, NULL, g_hInstance, NULL);
          SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
          
          HWND hChk1 = CreateWindowExW(0, L"Button", L"System Tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TRAY + mon.index), g_hInstance, NULL);
          HWND hChk2 = CreateWindowExW(0, L"Button", L"Clock", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 120, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_CLOCK + mon.index), g_hInstance, NULL);
          HWND hChk3 = CreateWindowExW(0, L"Button", L"Task Buttons", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 40, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TBTN + mon.index), g_hInstance, NULL);
          
          SendMessageW(hChk1, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hChk2, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hChk3, WM_SETFONT, (WPARAM)hFont, 0);
          
          HWND hLblMode = CreateWindowExW(0, L"Static", L"Start Menu Mode:", WS_CHILD | WS_VISIBLE, 15, y + 65, 120, 15, hScroll, NULL, g_hInstance, NULL);
          HWND hLblTrig = CreateWindowExW(0, L"Static", L"Start Menu Trigger:", WS_CHILD | WS_VISIBLE, 15, y + 90, 120, 15, hScroll, NULL, g_hInstance, NULL);
          HWND hLblOrb = CreateWindowExW(0, L"Static", L"Start Orb Theme:", WS_CHILD | WS_VISIBLE, 15, y + 115, 120, 15, hScroll, NULL, g_hInstance, NULL);

          SendMessageW(hLblMode, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hLblTrig, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hLblOrb, WM_SETFONT, (WPARAM)hFont, 0);

          HWND hCmbMode = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 60, 100, 100, hScroll, (HMENU)(ID_BASE_SM_MODE + mon.index), g_hInstance, NULL);
          HWND hCmbTrig = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 85, 100, 100, hScroll, (HMENU)(ID_BASE_SM_TRIG + mon.index), g_hInstance, NULL);
          HWND hCmbOrb = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 110, 100, 100, hScroll, (HMENU)(ID_BASE_SM_ORB + mon.index), g_hInstance, NULL);
          HWND hPreview = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | WS_BORDER, 250, y + 100, 54, 54, hScroll, (HMENU)(ID_BASE_SM_PREV + mon.index), g_hInstance, NULL);

          AddTooltip(hScroll, hChk1, L"Show system tray icons on this monitor.");
          AddTooltip(hScroll, hChk2, L"Show clock widget on this monitor.");
          AddTooltip(hScroll, hChk3, L"Show application task buttons on this monitor.");
          AddTooltip(hScroll, hCmbMode, L"Choose which Start Menu to open on this monitor.");
          AddTooltip(hScroll, hCmbTrig, L"Select mouse/keyboard trigger to summon the Start Menu.");
          AddTooltip(hScroll, hCmbOrb, L"Pick the graphic theme for your Start Orb.");
          AddTooltip(hScroll, hPreview, L"A preview of your selected Start Orb theme.");

          SetWindowSubclass(hCmbMode, NoMouseWheelSubclassProc, 1, 0);
          SetWindowSubclass(hCmbTrig, NoMouseWheelSubclassProc, 1, 0);
          SetWindowSubclass(hCmbOrb, NoMouseWheelSubclassProc, 1, 0);

          SendMessageW(hCmbMode, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hCmbTrig, WM_SETFONT, (WPARAM)hFont, 0);
          SendMessageW(hCmbOrb, WM_SETFONT, (WPARAM)hFont, 0);

          SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Native Injection");
          SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
          SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Elite Custom Menu");
          SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Standalone");

          SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Left Click");
          SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Middle Click");
          SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Win Key");

          PopulateOrbComboBox(hCmbOrb);
          
          if (hKey) {
              DWORD dwValue = 0, cbDataVal = sizeof(DWORD);
              WCHAR val[64];
              wsprintfW(val, L"EnableTray_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbDataVal) == ERROR_SUCCESS && dwValue) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
              else if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
              
              cbDataVal = sizeof(DWORD);
              wsprintfW(val, L"EnableClock_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbDataVal) == ERROR_SUCCESS && dwValue) SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
              else SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
              
              cbDataVal = sizeof(DWORD);
              wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbDataVal) == ERROR_SUCCESS && dwValue) SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
              else SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);

              DWORD mode = 0; cbDataVal = sizeof(DWORD);
              wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&mode, &cbDataVal) != ERROR_SUCCESS) {
                  cbDataVal = sizeof(DWORD);
                  RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&mode, &cbDataVal);
              }
              SendMessageW(hCmbMode, CB_SETCURSEL, mode, 0);

              DWORD trig = 0; cbDataVal = sizeof(DWORD);
              wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&trig, &cbDataVal) != ERROR_SUCCESS) {
                  cbDataVal = sizeof(DWORD);
                  RegQueryValueExW(hKey, L"StartMenuTrigger", NULL, NULL, (LPBYTE)&trig, &cbDataVal);
              }
              SendMessageW(hCmbTrig, CB_SETCURSEL, trig, 0);

              DWORD orb = 0; cbDataVal = sizeof(DWORD);
              wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
              if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&orb, &cbDataVal) == ERROR_SUCCESS) {
                  SelectOrbComboBox(hCmbOrb, orb);
              } else {
                  SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                  orb = IDB_START_ORB;
              }
              HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orb);
              SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
          } else {
              SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
              HBITMAP hBitmap = LoadPngResourceAsHBITMAP(IDB_START_ORB);
              SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
              if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
              SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
              SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
              SendMessageW(hCmbMode, CB_SETCURSEL, 0, 0);
              SendMessageW(hCmbTrig, CB_SETCURSEL, 0, 0);
          }
          y += 185;
      }
  }
  if (hKey) RegCloseKey(hKey);
  SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE, 0, y, 200 };
  SetScrollInfo(hScroll, SB_VERT, &si, TRUE);
  return TRUE;
  ```
- **Under `WM_NOTIFY` / `PSN_APPLY`**:
  ```cpp
  HKEY hKey;
  HKEY hKeyRoot = GetEliteRegistryRoot();
  if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
      // Load current layout mode
      DWORD migrateVal = 1;
      DWORD cbData = sizeof(DWORD);
      RegQueryValueExW(hKey, L"MigrateStartMenuSettings", NULL, NULL, (LPBYTE)&migrateVal, &cbData);

      for (const auto& mon : g_Monitors) {
          HWND hChk1 = GetDlgItem(hScroll, ID_BASE_MM_TRAY + mon.index);
          HWND hChk2 = GetDlgItem(hScroll, ID_BASE_MM_CLOCK + mon.index);
          HWND hChk3 = GetDlgItem(hScroll, ID_BASE_MM_TBTN + mon.index);
          
          if (hChk1 && hChk2 && hChk3) {
              DWORD v1 = (SendMessageW(hChk1, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
              DWORD v2 = (SendMessageW(hChk2, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
              DWORD v3 = (SendMessageW(hChk3, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
              
              WCHAR val[64];
              wsprintfW(val, L"EnableTray_Mon%d", mon.index);
              RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v1, sizeof(DWORD));
              wsprintfW(val, L"EnableClock_Mon%d", mon.index);
              RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v2, sizeof(DWORD));
              wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
              RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v3, sizeof(DWORD));
          }

          if (!migrateVal) {
              // Only save these from MultiMon tab if we aren't migrating
              HWND hCmbMode = GetDlgItem(hScroll, ID_BASE_SM_MODE + mon.index);
              HWND hCmbTrig = GetDlgItem(hScroll, ID_BASE_SM_TRIG + mon.index);
              HWND hCmbOrb = GetDlgItem(hScroll, ID_BASE_SM_ORB + mon.index);
              if (hCmbMode && hCmbTrig && hCmbOrb) {
                  DWORD mode = SendMessageW(hCmbMode, CB_GETCURSEL, 0, 0);
                  DWORD trig = SendMessageW(hCmbTrig, CB_GETCURSEL, 0, 0);
                  int sel = SendMessageW(hCmbOrb, CB_GETCURSEL, 0, 0);
                  
                  WCHAR val[64];
                  wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                  RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                  if (mon.index == 0) RegSetValueExW(hKey, L"StartMenuMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                  
                  wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                  RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                  if (mon.index == 0) RegSetValueExW(hKey, L"StartMenuTrigger", 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                  
                  if (sel != CB_ERR) {
                      DWORD orbId = SendMessageW(hCmbOrb, CB_GETITEMDATA, sel, 0);
                      wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                      RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                      if (mon.index == 0) RegSetValueExW(hKey, L"StartOrbID", 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                  }
              }
          }
      }
      RegCloseKey(hKey);
  }
  ```

### E. Start Menu Dialog Proc (`SourceFiles/TaskbarProperties.cpp` and `Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp`)
Expose the dynamic monitor controls layout when `MigrateStartMenuSettings = 1`:
```cpp
INT_PTR CALLBACK StartMenuSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hScroll = NULL;
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        AddDlgTooltip(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, L"Enable Open-Shell integration fallback when in replace mode. Classic Start Menu experience.");
        AddDlgTooltip(hwndDlg, IDC_MIGRATE_START_MENU_SETTINGS, L"Migrate per-monitor Start Menu settings to this tab from Multi-Monitor tab.");
        
        hScroll = CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
        
        HKEY hKey;
        DWORD dwValue = 1; // Default to 1
        DWORD migrateVal = 1; // Default to 1
        DWORD cbData = sizeof(DWORD);
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"FallbackStartMenuEnabled", NULL, NULL, (LPBYTE)&dwValue, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"MigrateStartMenuSettings", NULL, NULL, (LPBYTE)&migrateVal, &cbData);
            RegCloseKey(hKey);
        }
        SendDlgItemMessageW(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_MIGRATE_START_MENU_SETTINGS, BM_SETCHECK, migrateVal ? BST_CHECKED : BST_UNCHECKED, 0);
        
        if (migrateVal) {
            if (g_Monitors.empty()) EnumDisplayMonitors(NULL, NULL, TaskbarPropsMonitorEnumProc, 0);
            int y = 5;
            HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
            
            HKEY hKeyRead = NULL;
            RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKeyRead);
            
            for (const auto& mon : g_Monitors) {
                WCHAR title[64];
                wsprintfW(title, L"Monitor %d Start Menu", mon.index);
                HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, 105, hScroll, NULL, g_hInstance, NULL);
                SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
                
                HWND hLblMode = CreateWindowExW(0, L"Static", L"Start Menu Mode:", WS_CHILD | WS_VISIBLE, 15, y + 20, 120, 15, hScroll, NULL, g_hInstance, NULL);
                HWND hLblTrig = CreateWindowExW(0, L"Static", L"Start Menu Trigger:", WS_CHILD | WS_VISIBLE, 15, y + 45, 120, 15, hScroll, NULL, g_hInstance, NULL);
                HWND hLblOrb = CreateWindowExW(0, L"Static", L"Start Orb Theme:", WS_CHILD | WS_VISIBLE, 15, y + 70, 120, 15, hScroll, NULL, g_hInstance, NULL);
                
                SendMessageW(hLblMode, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hLblTrig, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hLblOrb, WM_SETFONT, (WPARAM)hFont, 0);
                
                HWND hCmbMode = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 15, 160, 100, hScroll, (HMENU)(ID_BASE_SM_MODE + mon.index), g_hInstance, NULL);
                HWND hCmbTrig = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 40, 160, 100, hScroll, (HMENU)(ID_BASE_SM_TRIG + mon.index), g_hInstance, NULL);
                HWND hCmbOrb = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 65, 110, 100, hScroll, (HMENU)(ID_BASE_SM_ORB + mon.index), g_hInstance, NULL);
                HWND hPreview = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | WS_BORDER, 260, y + 60, 40, 40, hScroll, (HMENU)(ID_BASE_SM_PREV + mon.index), g_hInstance, NULL);
                
                AddTooltip(hScroll, hCmbMode, L"Choose which Start Menu to open on this monitor.");
                AddTooltip(hScroll, hCmbTrig, L"Select mouse/keyboard trigger to summon the Start Menu.");
                AddTooltip(hScroll, hCmbOrb, L"Pick the graphic theme for your Start Orb.");
                AddTooltip(hScroll, hPreview, L"A preview of your selected Start Orb theme.");
                
                SetWindowSubclass(hCmbMode, NoMouseWheelSubclassProc, 1, 0);
                SetWindowSubclass(hCmbTrig, NoMouseWheelSubclassProc, 1, 0);
                SetWindowSubclass(hCmbOrb, NoMouseWheelSubclassProc, 1, 0);
                
                SendMessageW(hCmbMode, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hCmbTrig, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hCmbOrb, WM_SETFONT, (WPARAM)hFont, 0);
                
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Native Injection");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Elite Custom Menu");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Standalone");
                
                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Left Click");
                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Middle Click");
                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Win Key");
                
                PopulateOrbComboBox(hCmbOrb);
                
                if (hKeyRead) {
                    DWORD mode = 0, trig = 0, orb = 0;
                    DWORD cbSize = sizeof(DWORD);
                    WCHAR val[64];
                    
                    wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                    if (RegQueryValueExW(hKeyRead, val, NULL, NULL, (LPBYTE)&mode, &cbSize) != ERROR_SUCCESS) {
                        cbSize = sizeof(DWORD);
                        RegQueryValueExW(hKeyRead, L"StartMenuMode", NULL, NULL, (LPBYTE)&mode, &cbSize);
                    }
                    SendMessageW(hCmbMode, CB_SETCURSEL, mode, 0);
                    
                    cbSize = sizeof(DWORD);
                    wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                    if (RegQueryValueExW(hKeyRead, val, NULL, NULL, (LPBYTE)&trig, &cbSize) != ERROR_SUCCESS) {
                        cbSize = sizeof(DWORD);
                        RegQueryValueExW(hKeyRead, L"StartMenuTrigger", NULL, NULL, (LPBYTE)&trig, &cbSize);
                    }
                    SendMessageW(hCmbTrig, CB_SETCURSEL, trig, 0);
                    
                    cbSize = sizeof(DWORD);
                    wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                    if (RegQueryValueExW(hKeyRead, val, NULL, NULL, (LPBYTE)&orb, &cbSize) == ERROR_SUCCESS) {
                        SelectOrbComboBox(hCmbOrb, orb);
                    } else {
                        SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                        orb = IDB_START_ORB;
                    }
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orb);
                    SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                } else {
                    SendMessageW(hCmbMode, CB_SETCURSEL, 0, 0);
                    SendMessageW(hCmbTrig, CB_SETCURSEL, 0, 0);
                    SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(IDB_START_ORB);
                    SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                }
                y += 115;
            }
            if (hKeyRead) RegCloseKey(hKeyRead);
            SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE, 0, y, 170 };
            SetScrollInfo(hScroll, SB_VERT, &si, TRUE);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            
            int id = LOWORD(wParam);
            if (HIWORD(wParam) == CBN_SELCHANGE && id >= ID_BASE_SM_ORB && id < ID_BASE_SM_ORB + 32) {
                int monIndex = id - ID_BASE_SM_ORB;
                HWND hCombo = GetDlgItem(hScroll, id);
                int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    DWORD orbId = SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
                    HWND hPreview = GetDlgItem(hScroll, ID_BASE_SM_PREV + monIndex);
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orbId);
                    HBITMAP hOld = (HBITMAP)SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                    if (hOld) DeleteObject(hOld);
                }
            }
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            HKEY hKeyRoot = GetEliteRegistryRoot();
            if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD fallbackVal = (SendDlgItemMessageW(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"FallbackStartMenuEnabled", 0, REG_DWORD, (const BYTE*)&fallbackVal, sizeof(DWORD));
                
                DWORD migrateVal = (SendDlgItemMessageW(hwndDlg, IDC_MIGRATE_START_MENU_SETTINGS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"MigrateStartMenuSettings", 0, REG_DWORD, (const BYTE*)&migrateVal, sizeof(DWORD));
                
                if (migrateVal) {
                    for (const auto& mon : g_Monitors) {
                        HWND hCmbMode = GetDlgItem(hScroll, ID_BASE_SM_MODE + mon.index);
                        HWND hCmbTrig = GetDlgItem(hScroll, ID_BASE_SM_TRIG + mon.index);
                        HWND hCmbOrb = GetDlgItem(hScroll, ID_BASE_SM_ORB + mon.index);
                        
                        if (hCmbMode && hCmbTrig && hCmbOrb) {
                            DWORD mode = SendMessageW(hCmbMode, CB_GETCURSEL, 0, 0);
                            DWORD trig = SendMessageW(hCmbTrig, CB_GETCURSEL, 0, 0);
                            int sel = SendMessageW(hCmbOrb, CB_GETCURSEL, 0, 0);
                            
                            WCHAR val[64];
                            wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                            RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                            if (mon.index == 0) RegSetValueExW(hKey, L"StartMenuMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                            
                            wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                            RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                            if (mon.index == 0) RegSetValueExW(hKey, L"StartMenuTrigger", 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                            
                            if (sel != CB_ERR) {
                                DWORD orbId = SendMessageW(hCmbOrb, CB_GETITEMDATA, sel, 0);
                                wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                                RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                                if (mon.index == 0) RegSetValueExW(hKey, L"StartOrbID", 0, REG_DWORD, (const BYTE*)&orbId, sizeof(DWORD));
                            }
                        }
                    }
                }
                
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}
```

## 6. Verification Method
1. Build the project using `build.ps1` with the `$env:ELITE_AUDITOR_RUN = "1"` environment variable.
2. Launch `EliteSettings.exe` (or open the Control Panel Applet `EliteSettings.cpl` via `rundll32.exe Shell32.dll,Control_RunDLL EliteSettings.cpl`).
3. Click the "Start Menu" tab.
4. Verify that:
   - The checkbox "Use Fallback Start Menu (Open-Shell Integration)" and the checkbox "Migrate per-monitor settings from Multi-Monitor tab" are visible instantly without requiring mouse hover.
   - The per-monitor Start Menu settings (Start Menu Mode, Start Menu Trigger, Start Orb Theme, Preview) are loaded and populated in the scroll area by default.
5. Uncheck "Migrate per-monitor settings from Multi-Monitor tab" and click "Apply". Verify that the scroll area in the Start Menu tab disappears/becomes empty, and the per-monitor settings reappear inside the "Multi-Monitor Components" tab dialog.
6. Verify that checking it back moves them to the Start Menu tab again.
