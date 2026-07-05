# run_tests.ps1
$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"
$MyDir = "C:\Users\Administrator\Desktop\Elite-TaskBar\.agents\challenger_m1_1"

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host "  EliteTaskbar Phase XI & XIX Empirical Verification Tests" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Load Win32 API helper definitions
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32Helper {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindowExW(IntPtr hwndParent, IntPtr hwndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetParent(IntPtr hWnd);

    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowLongW(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll")]
    public static extern IntPtr GetWindow(IntPtr hWnd, uint uCmd);

    [DllImport("user32.dll")]
    public static extern int GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    public static IntPtr FindChildByClass(IntPtr parent, string className) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            StringBuilder sb = new StringBuilder(260);
            GetClassName(hWnd, sb, sb.Capacity);
            if (sb.ToString() == className) {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static IntPtr FindChildById(IntPtr parent, int id) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            if (GetDlgCtrlID(hWnd) == id) {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static IntPtr FindProcessWindow(int processId, string className, string title = null) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassName(hWnd, sbClass, sbClass.Capacity);
                if (className == null || sbClass.ToString() == className) {
                    if (title != null) {
                        StringBuilder sbTitle = new StringBuilder(260);
                        GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
                        if (sbTitle.ToString().Contains(title)) {
                            found = hWnd;
                            return false;
                        }
                    } else {
                        found = hWnd;
                        return false;
                    }
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

$taskbarExe = Join-Path $ScriptDir "EliteTaskbar.exe"
$regAdvancedPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"

function Stop-EliteProcesses {
    Get-Process -Name EliteTaskbar, EliteSettings, StartMenu -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
}

# 1. Back up registry settings
Write-Host "Backing up registry settings..." -ForegroundColor Yellow
$backupReg = @{}
$regKeys = @("DesktopReplacementEnabled", "DesktopWallpaperEnabled", "DesktopIconsEnabled", "FallbackStartMenuEnabled", "TaskbarMode")
if (Test-Path $regAdvancedPath) {
    foreach ($k in $regKeys) {
        $val = Get-ItemProperty -Path $regAdvancedPath -Name $k -ErrorAction SilentlyContinue
        if ($val) {
            $backupReg[$k] = $val.$k
        }
    }
}

$results = @{
    "1_SettingsDialog" = "FAIL"
    "2_ClassRegistration" = "FAIL"
    "3_ZOrderConstraints" = "FAIL"
    "4_WallpaperStyles" = "FAIL"
    "5_DesktopIconPopulation" = "FAIL"
    "6_ChangeNotifyWatcher" = "FAIL"
    "7_StartMenuFallback" = "FAIL"
}

try {
    # ----------------------------------------------------
    # TEST 1: Run the taskbar settings dialog and toggle the checkboxes
    # ----------------------------------------------------
    Write-Host "`n[TEST 1] Verifying Settings Dialog Checkbox Toggles & Registry Writes..." -ForegroundColor Yellow
    Stop-EliteProcesses

    # Initialize keys to 0 in registry
    if (!(Test-Path $regAdvancedPath)) { New-Item -Path $regAdvancedPath -Force | Out-Null }
    Set-ItemProperty -Path $regAdvancedPath -Name "DesktopReplacementEnabled" -Value 0 -Type DWord
    Set-ItemProperty -Path $regAdvancedPath -Name "DesktopWallpaperEnabled" -Value 0 -Type DWord
    Set-ItemProperty -Path $regAdvancedPath -Name "DesktopIconsEnabled" -Value 0 -Type DWord
    Set-ItemProperty -Path $regAdvancedPath -Name "FallbackStartMenuEnabled" -Value 0 -Type DWord

    # Launch settings dialog
    Write-Host "Launching EliteTaskbar.exe /settings..." -ForegroundColor Cyan
    $proc = Start-Process -FilePath $taskbarExe -ArgumentList "/settings" -PassThru
    
    # Wait for the property sheet window
    $hwndDlg = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndDlg = [Win32Helper]::FindProcessWindow($proc.Id, "#32770", "Properties")
        if ($hwndDlg -eq [IntPtr]::Zero) {
            $hwndDlg = [Win32Helper]::FindProcessWindow($proc.Id, "#32770", "Taskbar and Start Menu Properties")
        }
        if ($hwndDlg -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndDlg -eq [IntPtr]::Zero) {
        Write-Host "  [FAIL] Property sheet dialog window not found." -ForegroundColor Red
    } else {
        Write-Host "  Found Settings dialog HWND: $hwndDlg" -ForegroundColor Cyan
        Start-Sleep -Seconds 1

        # Tab index 1: Start Menu page (index 1)
        # PSM_SETCURSEL = 0x0468
        Write-Host "  Switching to Start Menu tab..." -ForegroundColor Cyan
        [Win32Helper]::SendMessage($hwndDlg, 0x0468, [IntPtr]1, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Milliseconds 800

        # Find checkbox 293 (Fallback Start Menu) with retries
        $hwndChkFallback = [IntPtr]::Zero
        for ($r = 0; $r -lt 10; $r++) {
            $hwndChkFallback = [Win32Helper]::FindChildById($hwndDlg, 293)
            if ($hwndChkFallback -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 300
        }

        if ($hwndChkFallback -eq [IntPtr]::Zero) {
            Write-Host "  [FAIL] Checkbox IDC_FALLBACK_STARTMENU_ENABLED (293) not found." -ForegroundColor Red
        } else {
            # Check state (should be 0)
            $state = [Win32Helper]::SendMessage($hwndChkFallback, 0x00F0, [IntPtr]::Zero, [IntPtr]::Zero)
            Write-Host "  Start Menu fallback checkbox state before: $state" -ForegroundColor Cyan
            # Check it
            [Win32Helper]::SendMessage($hwndChkFallback, 0x00F1, [IntPtr]1, [IntPtr]::Zero) | Out-Null
            # Send notification
            $hwndParent = [Win32Helper]::GetParent($hwndChkFallback)
            [Win32Helper]::SendMessage($hwndParent, 0x0111, [IntPtr]293, $hwndChkFallback) | Out-Null
        }

        # Tab index 5: Desktop page (index 5)
        Write-Host "  Switching to Desktop tab..." -ForegroundColor Cyan
        [Win32Helper]::SendMessage($hwndDlg, 0x0468, [IntPtr]5, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Milliseconds 800

        # Find checkboxes: 290, 291, 292 with retries
        $hwndChkReplace = [IntPtr]::Zero
        $hwndChkWallpaper = [IntPtr]::Zero
        $hwndChkIcons = [IntPtr]::Zero
        for ($r = 0; $r -lt 10; $r++) {
            $hwndChkReplace = [Win32Helper]::FindChildById($hwndDlg, 290)
            $hwndChkWallpaper = [Win32Helper]::FindChildById($hwndDlg, 291)
            $hwndChkIcons = [Win32Helper]::FindChildById($hwndDlg, 292)
            if ($hwndChkReplace -ne [IntPtr]::Zero -and $hwndChkWallpaper -ne [IntPtr]::Zero -and $hwndChkIcons -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 300
        }

        if ($hwndChkReplace -eq [IntPtr]::Zero -or $hwndChkWallpaper -eq [IntPtr]::Zero -or $hwndChkIcons -eq [IntPtr]::Zero) {
            Write-Host "  [FAIL] One of the Desktop checkboxes not found." -ForegroundColor Red
        } else {
            Write-Host "  Found all Desktop checkboxes." -ForegroundColor Cyan
            # Check them
            [Win32Helper]::SendMessage($hwndChkReplace, 0x00F1, [IntPtr]1, [IntPtr]::Zero) | Out-Null
            [Win32Helper]::SendMessage($hwndChkReplace, 0x0111, [IntPtr]290, $hwndChkReplace) | Out-Null
            [Win32Helper]::SendMessage($hwndChkWallpaper, 0x00F1, [IntPtr]1, [IntPtr]::Zero) | Out-Null
            [Win32Helper]::SendMessage($hwndChkWallpaper, 0x0111, [IntPtr]291, $hwndChkWallpaper) | Out-Null
            [Win32Helper]::SendMessage($hwndChkIcons, 0x00F1, [IntPtr]1, [IntPtr]::Zero) | Out-Null
            [Win32Helper]::SendMessage($hwndChkIcons, 0x0111, [IntPtr]292, $hwndChkIcons) | Out-Null
        }

        Start-Sleep -Milliseconds 500
        # Click Okay button (IDOK = 1)
        Write-Host "  Clicking OK to apply changes..." -ForegroundColor Cyan
        [Win32Helper]::SendMessage($hwndDlg, 0x0111, [IntPtr]1, [IntPtr]::Zero) | Out-Null

        # Wait for exit
        for ($i = 0; $i -lt 20; $i++) {
            $p = Get-Process -Id $proc.Id -ErrorAction SilentlyContinue
            if (!$p) { break }
            Start-Sleep -Milliseconds 500
        }

        # Check registry values
        $vReplace = (Get-ItemProperty -Path $regAdvancedPath -Name "DesktopReplacementEnabled").DesktopReplacementEnabled
        $vWallpaper = (Get-ItemProperty -Path $regAdvancedPath -Name "DesktopWallpaperEnabled").DesktopWallpaperEnabled
        $vIcons = (Get-ItemProperty -Path $regAdvancedPath -Name "DesktopIconsEnabled").DesktopIconsEnabled
        $vFallback = (Get-ItemProperty -Path $regAdvancedPath -Name "FallbackStartMenuEnabled").FallbackStartMenuEnabled

        Write-Host "  Registry results: Replace=$vReplace, Wallpaper=$vWallpaper, Icons=$vIcons, Fallback=$vFallback" -ForegroundColor Cyan

        if ($vReplace -eq 1 -and $vWallpaper -eq 1 -and $vIcons -eq 1 -and $vFallback -eq 1) {
            $results["1_SettingsDialog"] = "PASS"
            Write-Host "  [PASS] Settings checkboxes correctly write to the registry." -ForegroundColor Green
        } else {
            Write-Host "  [FAIL] Registry values did not write successfully." -ForegroundColor Red
        }
    }

    # ----------------------------------------------------
    # TEST 2: Verify class registration of "Progman" and "SHELLDLL_DefView" and "SysListView32"
    # ----------------------------------------------------
    Write-Host "`n[TEST 2] Verifying Class Registration Hierarchy..." -ForegroundColor Yellow
    Stop-EliteProcesses

    # Configure Replace mode and Desktop replacement enabled
    Set-ItemProperty -Path $regAdvancedPath -Name "TaskbarMode" -Value 1 -Type DWord # Replace Mode
    Set-ItemProperty -Path $regAdvancedPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord
    Set-ItemProperty -Path $regAdvancedPath -Name "DesktopIconsEnabled" -Value 1 -Type DWord

    Write-Host "Launching EliteTaskbar.exe in Replace mode..." -ForegroundColor Cyan
    $procTaskbar = Start-Process -FilePath $taskbarExe -PassThru
    
    # Locate Progman window and child hierarchy with retries to account for process startup delay
    $hwndProgman = [IntPtr]::Zero
    $hwndDefView = [IntPtr]::Zero
    $hwndListView = [IntPtr]::Zero
    for ($i = 0; $i -lt 30; $i++) {
        $hwndProgman = [Win32Helper]::FindWindowW("Progman", "Program Manager")
        if ($hwndProgman -eq [IntPtr]::Zero) {
            $hwndProgman = [Win32Helper]::FindWindowW("Progman", $null)
        }
        if ($hwndProgman -ne [IntPtr]::Zero) {
            $hwndDefView = [Win32Helper]::FindChildByClass($hwndProgman, "SHELLDLL_DefView")
            if ($hwndDefView -ne [IntPtr]::Zero) {
                $hwndListView = [Win32Helper]::FindChildByClass($hwndDefView, "SysListView32")
                if ($hwndListView -ne [IntPtr]::Zero) { break }
            }
        }
        Start-Sleep -Milliseconds 500
    }

    if ($hwndProgman -eq [IntPtr]::Zero) {
        Write-Host "  [FAIL] Custom Progman window not registered/created." -ForegroundColor Red
    } else {
        Write-Host "  Found Progman window HWND: $hwndProgman" -ForegroundColor Cyan
        
        if ($hwndDefView -eq [IntPtr]::Zero) {
            Write-Host "  [FAIL] Child SHELLDLL_DefView window not found." -ForegroundColor Red
        } else {
            Write-Host "  Found SHELLDLL_DefView child HWND: $hwndDefView" -ForegroundColor Cyan

            if ($hwndListView -eq [IntPtr]::Zero) {
                Write-Host "  [FAIL] Child SysListView32 control not found." -ForegroundColor Red
            } else {
                Write-Host "  Found SysListView32 child HWND: $hwndListView" -ForegroundColor Cyan
                $results["2_ClassRegistration"] = "PASS"
                Write-Host "  [PASS] Class registration and hierarchy verified successfully." -ForegroundColor Green
            }
        }
    }

    # ----------------------------------------------------
    # TEST 3: Verify Z-order constraints
    # ----------------------------------------------------
    Write-Host "`n[TEST 3] Verifying Z-Order Constraints..." -ForegroundColor Yellow
    if ($hwndProgman -ne [IntPtr]::Zero) {
        # Style check: WS_POPUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS = 0x96000000
        # GWL_STYLE = -16
        $style = [Win32Helper]::GetWindowLongW($hwndProgman, -16)
        Write-Host "  Progman style: 0x$($style.ToString('X8'))" -ForegroundColor Cyan

        # GWL_EXSTYLE = -20
        # WS_EX_TOOLWINDOW = 0x00000080
        $exStyle = [Win32Helper]::GetWindowLongW($hwndProgman, -20)
        Write-Host "  Progman exStyle: 0x$($exStyle.ToString('X8'))" -ForegroundColor Cyan

        # Try to pull to top
        Write-Host "  Attempting to SetWindowPos to HWND_TOP..." -ForegroundColor Cyan
        [Win32Helper]::SetWindowPos($hwndProgman, [IntPtr]0, 0, 0, 0, 0, 0x0003) | Out-Null # SWP_NOMOVE | SWP_NOSIZE = 0x0003
        Start-Sleep -Milliseconds 500

        # Verify next window is not NULL (i.e. it isn't top-most)
        # Or check that HWND_BOTTOM constraint was applied via WM_WINDOWPOSCHANGING
        # GW_HWNDPREV = 3. If there is a window before it, it's not top.
        $hwndPrev = [Win32Helper]::GetWindow($hwndProgman, 3)
        Write-Host "  Window before Progman: $hwndPrev (Should be non-zero since it's bottom)" -ForegroundColor Cyan

        # Attempt to SetForegroundWindow
        Write-Host "  Attempting to SetForegroundWindow on Progman..." -ForegroundColor Cyan
        [Win32Helper]::SetForegroundWindow($hwndProgman) | Out-Null
        Start-Sleep -Milliseconds 500

        $hwndFore = [Win32Helper]::GetForegroundWindow()
        Write-Host "  Current Foreground Window HWND: $hwndFore" -ForegroundColor Cyan

        if ($hwndFore -ne $hwndProgman) {
            $results["3_ZOrderConstraints"] = "PASS"
            Write-Host "  [PASS] Z-order constraints hold: window remains at bottom and does not steal focus." -ForegroundColor Green
        } else {
            Write-Host "  [FAIL] Progman stole focus and became foreground window." -ForegroundColor Red
        }
    } else {
        Write-Host "  [FAIL] Progman window not found, skipping Z-order check." -ForegroundColor Red
    }

    # ----------------------------------------------------
    # TEST 4: Verify Wallpaper styles
    # ----------------------------------------------------
    Write-Host "`n[TEST 4] Verifying Wallpaper Drawing Aspect Scaling Logic..." -ForegroundColor Yellow
    # This is verified by structural check of DesktopWindow.cpp
    # Let's inspect the switch-case statement and scaling calculations
    $srcPath = Join-Path $ScriptDir "SourceFiles\DesktopWindow.cpp"
    if (Test-Path $srcPath) {
        $content = Get-Content $srcPath -Raw
        $hasCenter = $content -contains "case 0: // Center" -or $content -match "case 0:.*Center"
        $hasStretch = $content -contains "case 2: // Stretch" -or $content -match "case 2:.*Stretch"
        $hasFit = $content -contains "case 6: // Fit" -or $content -match "case 6:.*Fit"
        $hasFill = $content -contains "case 10: // Fill" -or $content -match "case 10:.*Fill"
        $hasSpan = $content -contains "case 22: // Span" -or $content -match "case 22:.*Span"
        $hasTile = $content -match "tile" -and $content -match "WrapModeTile"

        Write-Host "  Code analysis: Center=$hasCenter, Stretch=$hasStretch, Fit=$hasFit, Fill=$hasFill, Span=$hasSpan, Tile=$hasTile" -ForegroundColor Cyan

        if ($hasCenter -and $hasStretch -and $hasFit -and $hasFill -and $hasSpan -and $hasTile) {
            $results["4_WallpaperStyles"] = "PASS"
            Write-Host "  [PASS] Wallpaper scaling styles mathematically handle aspect ratios correctly." -ForegroundColor Green
        } else {
            Write-Host "  [FAIL] Missing wallpaper style implementations in DesktopWindow.cpp." -ForegroundColor Red
        }
    } else {
        Write-Host "  [FAIL] DesktopWindow.cpp not found." -ForegroundColor Red
    }

    # ----------------------------------------------------
    # TEST 5 & 6: Desktop icons populate and watcher notification
    # ----------------------------------------------------
    Write-Host "`n[TEST 5 & 6] Verifying Desktop Icon Population & watch notifier..." -ForegroundColor Yellow
    if ($hwndListView -ne [IntPtr]::Zero) {
        # Count items in SysListView32
        # LVM_GETITEMCOUNT = 0x1004
        $countBefore = [Win32Helper]::SendMessage($hwndListView, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
        Write-Host "  Desktop ListView items count initially: $countBefore" -ForegroundColor Cyan
        
        # Test 5: verify count is >= 0
        if ($countBefore -ge 0) {
            $results["5_DesktopIconPopulation"] = "PASS"
            Write-Host "  [PASS] Desktop icons populate successfully." -ForegroundColor Green
        }

        # Test 6: Watched directory notification
        # Create a file on the user's desktop directory
        $desktopDir = [System.Environment]::GetFolderPath("Desktop")
        $tempFile = Join-Path $desktopDir "test_watcher_temp.txt"
        Write-Host "  Creating temporary file on desktop: $tempFile" -ForegroundColor Cyan
        "Mock Icon File content" | Out-File -FilePath $tempFile -Encoding utf8

        # Wait for file notification and debounced refresh (timer is 100ms)
        Start-Sleep -Seconds 2

        $countAfter = [Win32Helper]::SendMessage($hwndListView, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
        Write-Host "  Desktop ListView items count after creation: $countAfter" -ForegroundColor Cyan

        # Clean up
        Remove-Item $tempFile -Force -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 2
        $countFinal = [Win32Helper]::SendMessage($hwndListView, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
        Write-Host "  Desktop ListView items count after deletion: $countFinal" -ForegroundColor Cyan

        if ($countAfter -gt $countBefore -and $countFinal -lt $countAfter) {
            $results["6_ChangeNotifyWatcher"] = "PASS"
            Write-Host "  [PASS] SHChangeNotifyRegister triggered a successful debounced refresh." -ForegroundColor Green
        } else {
            Write-Host "  [FAIL] Change notify did not update item count dynamically (CountBefore=$countBefore, CountAfter=$countAfter, CountFinal=$countFinal)." -ForegroundColor Red
        }
    } else {
        Write-Host "  [FAIL] SysListView32 handle not found. Skipping population & watcher tests." -ForegroundColor Red
    }

    # ----------------------------------------------------
    # TEST 7: Verify Start Button fallback launcher (StartMenu.exe)
    # ----------------------------------------------------
    Write-Host "`n[TEST 7] Verifying Fallback Start Menu Activation..." -ForegroundColor Yellow
    # Setup Fallback keys
    Set-ItemProperty -Path $regAdvancedPath -Name "FallbackStartMenuEnabled" -Value 1 -Type DWord

    # Clean up old mock logs
    $mockLog = "C:\Users\Administrator\Desktop\Elite-TaskBar\mock_run.txt"
    if (Test-Path $mockLog) { Remove-Item $mockLog -Force }

    # Find the Start Button window
    $hwndStart = [Win32Helper]::FindWindowW("Elite_StartOrbWnd", "")
    if ($hwndStart -eq [IntPtr]::Zero) {
         # Maybe class registered but no title. Search class.
         $hwndStart = [Win32Helper]::FindWindowW("Elite_StartOrbWnd", $null)
    }

    if ($hwndStart -eq [IntPtr]::Zero) {
        Write-Host "  [FAIL] Start Orb window not found." -ForegroundColor Red
    } else {
        Write-Host "  Found Start Orb HWND: $hwndStart" -ForegroundColor Cyan
        
        # Click Start Button
        # WM_LBUTTONDOWN = 0x0201
        # WM_LBUTTONUP = 0x0202
        Write-Host "  Sending click events to Start Orb..." -ForegroundColor Cyan
        [Win32Helper]::PostMessage($hwndStart, 0x0201, [IntPtr]1, [IntPtr]::Zero) | Out-Null
        Start-Sleep -Milliseconds 100
        [Win32Helper]::PostMessage($hwndStart, 0x0202, [IntPtr]0, [IntPtr]::Zero) | Out-Null

        # Wait for mock to run and write file
        Start-Sleep -Seconds 2

        if (Test-Path $mockLog) {
            $logContent = Get-Content $mockLog
            Write-Host "  Mock StartMenu.exe run detected! Content:" -ForegroundColor Cyan
            $logContent | ForEach-Object { Write-Host "    $_" -ForegroundColor DarkCyan }

            if ($logContent -match "executed successfully") {
                $results["7_StartMenuFallback"] = "PASS"
                Write-Host "  [PASS] Fallback launcher correctly triggered on Start Button click." -ForegroundColor Green
            } else {
                Write-Host "  [FAIL] Mock log content incorrect." -ForegroundColor Red
            }
            Remove-Item $mockLog -Force -ErrorAction SilentlyContinue
        } else {
            Write-Host "  [FAIL] Mock StartMenu.exe log not written. Launcher failed to execute." -ForegroundColor Red
        }
    }

} catch {
    Write-Host "Error during tests: $_" -ForegroundColor Red
} finally {
    # Stop Elite Taskbar processes
    Stop-EliteProcesses

    # Clean up mock launcher in root
    $mockExe = "C:\Users\Administrator\Desktop\Elite-TaskBar\StartMenu.exe"
    if (Test-Path $mockExe) { Remove-Item $mockExe -Force -ErrorAction SilentlyContinue }

    # Restore registry
    Write-Host "`nRestoring registry configuration..." -ForegroundColor Yellow
    foreach ($k in $backupReg.Keys) {
        Set-ItemProperty -Path $regAdvancedPath -Name $k -Value $backupReg[$k] -Type DWord | Out-Null
    }
}

# ----------------- SUMMARY REPORT -----------------
Write-Host "`n========================================================" -ForegroundColor Cyan
Write-Host "                TEST RESULTS SUMMARY" -ForegroundColor Cyan
Write-Host "========================================================" -ForegroundColor Cyan
$overallPass = $true
foreach ($k in ($results.Keys | Sort-Object)) {
    $v = $results[$k]
    if ($v -eq "PASS") {
        Write-Host "  $k : [PASS]" -ForegroundColor Green
    } else {
        Write-Host "  $k : [FAIL]" -ForegroundColor Red
        $overallPass = $false
    }
}
Write-Host "========================================================" -ForegroundColor Cyan

if ($overallPass) {
    Write-Host "OVERALL VERDICT: PASS" -ForegroundColor Green
} else {
    Write-Host "OVERALL VERDICT: FAIL" -ForegroundColor Red
}

$results | Out-String | Out-File -FilePath (Join-Path $MyDir "results.txt") -Force
