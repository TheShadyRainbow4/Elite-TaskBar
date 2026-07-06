## 2026-07-05T17:16:22Z
You are the Worker for Milestone 6 of the Elite-Taskbar project.
Your role: teamwork_preview_worker.
Your working directory: C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\worker_m6
Your parent is sub_orch_m6 (conversation ID: ce47fa31-c215-44a2-8083-787f37b736f5).

Your task is to implement the changes for all 4 features:
1. Start Menu Settings Tab Fix:
   - In both resource.h files (SourceFiles/resource.h and Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/resource.h), define:
     #define IDC_MIGRATE_START_MENU_SETTINGS 295
   - In both resources.rc files, update IDD_STARTMENU_PROPS:
     - Change coordinates/sizes of IDC_FALLBACK_STARTMENU_ENABLED to clear space.
     - Add the new checkbox:
       CONTROL "Migrate per-monitor settings from Multi-Monitor tab", IDC_MIGRATE_START_MENU_SETTINGS, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 7, 205, 238, 10
     - Ensure the template matches:
       IDD_STARTMENU_PROPS DIALOGEX 0, 0, 252, 218
       STYLE WS_CHILD | WS_DISABLED | WS_CAPTION | DS_CONTROL
       CAPTION "Start Menu"
       FONT 8, "Segoe UI Semibold"
       BEGIN
           LTEXT "Configure start menu per-monitor:", IDC_STATIC, 7, 7, 238, 8
           CONTROL "", IDC_DYN_SCROLLAREA, "Static", SS_BLACKFRAME | NOT WS_VISIBLE, 7, 20, 238, 170
           CONTROL "Use Fallback Start Menu (Open-Shell Integration)", IDC_FALLBACK_STARTMENU_ENABLED, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 7, 193, 238, 10
           CONTROL "Migrate per-monitor settings from Multi-Monitor tab", IDC_MIGRATE_START_MENU_SETTINGS, "Button", BS_AUTOCHECKBOX | WS_TABSTOP, 7, 205, 238, 10
       END
   - In both TaskbarProperties.cpp files (SourceFiles/TaskbarProperties.cpp and Win32Explorer_26.0.3.0/App_Source/EliteTaskbar/TaskbarProperties.cpp):
     - Fix CreateDynScrollArea bounds computation by using screen-to-client mapped coordinates:
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
     - Modify MultiMonSettingsDlgProc and StartMenuSettingsDlgProc to support the runtime migration toggle "MigrateStartMenuSettings" (defaulting to 1 if not present in registry).
       If MigrateStartMenuSettings is 1 (checked), per-monitor Start Menu comboboxes and previews are created in the Start Menu tab scroll container. MultiMon tab scroll container only displays "System Tray", "Clock", and "Task Buttons" checkboxes.
       If MigrateStartMenuSettings is 0 (unchecked), the legacy layout is preserved (Start Menu settings are in the MultiMon tab).
       Verify that combobox selections are saved and loaded correctly to registry key "MigrateStartMenuSettings" (under "Software\\EliteSoftware\\Win32Explorer\\Advanced" key using GetEliteRegistryRoot() root).

2. About Dialog Layout Fix:
   - In both TaskbarProperties.cpp files, update the expanded height chin boundary in WM_PAINT of the About dialog procedure (AboutPageDlgProc) to 192 (from 195).
   - In both TaskbarProperties.cpp files, update the expand/collapse resizing logic in WM_COMMAND (IDC_ABOUT_EXPAND) to dynamically compute window borders/caption size:
     RECT rcWindow, rcClient;
     GetWindowRect(hwndDlg, &rcWindow);
     GetClientRect(hwndDlg, &rcClient);
     int borderX = (rcWindow.right - rcWindow.left) - rcClient.right;
     int borderY = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
     And pass (rcDlg.right + borderX, rcDlg.bottom + borderY) to SetWindowPos in both expand and collapse branches.

3. Reload Win32Explorer Multi-Spawn Fix:
   - In both TaskbarProperties.cpp files, add a 1000ms debounce check to NotifySettingsChange():
     void NotifySettingsChange() {
         static ULONGLONG lastTriggerTime = 0;
         ULONGLONG currentTime = GetTickCount64();
         if (currentTime - lastTriggerTime < 1000) {
             return;
         }
         lastTriggerTime = currentTime;

         HANDLE hThread = CreateThread(NULL, 0, BroadcastSettingsChangeThread, NULL, 0, NULL);
         if (hThread) {
             CloseHandle(hThread);
         }
     }

4. Automatically Clean Up *old*.exe and *old*.cpl Files:
   - Add a cleanup loop at the end of build.ps1 before the final Write-Host "Done!" (around line 208) to delete all files matching *old*.exe, *Old*.exe, *old*.cpl, and *Old*.cpl in the workspace root, BuildOutput, and BuildOutputx86 directories.

5. Mirror and Build Verification:
   - Use powershell -File build.ps1 to build all variants. Parse the output and ensure there are no compilation errors.
   - Verify digital signing using elite-easysigner as configured in the build chain.
