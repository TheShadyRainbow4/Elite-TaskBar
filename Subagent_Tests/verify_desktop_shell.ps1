# Subagent_Tests\verify_desktop_shell.ps1
$ErrorActionPreference = 'Stop'

Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  ELITE DESKTOP SHELL & START MENU FALLBACK TESTS" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# Compile Win32 helper definitions
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;
using System.Threading;

public class DesktopShellTester {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr FindWindowExW(IntPtr hWndParent, IntPtr hWndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll")]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumChildProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumChildProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern IntPtr SendMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern int GetDlgCtrlID(IntPtr hwnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetParent(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();

    [DllImport("user32.dll")]
    public static extern IntPtr GetWindow(IntPtr hWnd, uint uCmd);

    [DllImport("user32.dll")]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    public const uint GW_HWNDLAST = 1;
    public const uint GW_HWNDNEXT = 2;

    public const uint WM_COMMAND = 0x0111;
    public const uint WM_NOTIFY = 0x004E;
    public const uint PSM_SETCURSEL = 0x0400 + 101;
    public const uint BM_SETCHECK = 0x00F1;
    public const uint BM_GETCHECK = 0x00F0;
    public const int BST_CHECKED = 1;
    public const int BST_UNCHECKED = 0;
    public const int BN_CLICKED = 0;
    public const int IDOK = 1;

    public const uint WM_LBUTTONDOWN = 0x0201;
    public const uint WM_LBUTTONUP = 0x0202;
    public const uint MK_LBUTTON = 0x0001;

    public static IntPtr FindPropSheetWindow(int processId) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassNameW(hWnd, sbClass, sbClass.Capacity);
                StringBuilder sbTitle = new StringBuilder(260);
                GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
                if (sbClass.ToString() == "#32770" && sbTitle.ToString().Contains("Properties")) {
                    found = hWnd;
                    return false;
                }
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static IntPtr FindChildById(IntPtr parent, int controlId) {
        IntPtr found = IntPtr.Zero;
        EnumChildWindows(parent, (hWnd, lParam) => {
            if (GetDlgCtrlID(hWnd) == controlId) {
                found = hWnd;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return found;
    }

    public static string GetWindowClassName(IntPtr hWnd) {
        StringBuilder sb = new StringBuilder(260);
        GetClassNameW(hWnd, sb, sb.Capacity);
        return sb.ToString();
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

$results = @{
    "SettingsRegistryToggles" = "FAIL"
    "DesktopStartupDynamic"   = "FAIL"
    "ClassRegistration"      = "FAIL"
    "ZOrderConstraints"      = "FAIL"
    "DesktopIconsLoading"    = "FAIL"
    "DirectoryChangeNotify"  = "FAIL"
    "StartButtonFallback"    = "FAIL"
}

# Cleanup existing test environments
Write-Host "0. Cleaning environment..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar, EliteSettings, Win32Explorer, StartMenu -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

# Registry Paths
$regPath = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
if (!(Test-Path $regPath)) { New-Item -Path $regPath -Force | Out-Null }

# ----------------- TEST 1: Settings Checkbox Toggles & Registry writing -----------------
Write-Host "`n[TEST 1] Testing Settings Dialog checkbox toggles..." -ForegroundColor Yellow

# Initialize registry values to 0
Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 0 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled" -Value 0 -Type DWord
Set-ItemProperty -Path $regPath -Name "DesktopIconsEnabled" -Value 0 -Type DWord
Set-ItemProperty -Path $regPath -Name "FallbackStartMenuEnabled" -Value 0 -Type DWord

$settingsProc = Start-Process -FilePath ".\EliteSettings.exe" -PassThru
Start-Sleep -Seconds 3

$hPropSheet = [DesktopShellTester]::FindPropSheetWindow($settingsProc.Id)
if ($hPropSheet -eq [IntPtr]::Zero) {
    Write-Host "[FAIL] Could not find EliteSettings properties sheet." -ForegroundColor Red
} else {
    Write-Host "Found settings properties sheet HWND: $hPropSheet" -ForegroundColor DarkCyan
    
    # Switch to Desktop tab (index 5)
    [DesktopShellTester]::SendMessageW($hPropSheet, [DesktopShellTester]::PSM_SETCURSEL, [IntPtr]5, [IntPtr]::Zero) | Out-Null
    Start-Sleep -Seconds 1
    
    # Find checkboxes on Desktop page
    $chkReplace = [DesktopShellTester]::FindChildById($hPropSheet, 290)
    $chkWallpaper = [DesktopShellTester]::FindChildById($hPropSheet, 291)
    $chkIcons = [DesktopShellTester]::FindChildById($hPropSheet, 292)
    $chkFallback = [DesktopShellTester]::FindChildById($hPropSheet, 293)
    
    if ($chkReplace -eq [IntPtr]::Zero -or $chkWallpaper -eq [IntPtr]::Zero -or $chkIcons -eq [IntPtr]::Zero -or $chkFallback -eq [IntPtr]::Zero) {
        Write-Host "[FAIL] Could not find checkboxes on Desktop settings page." -ForegroundColor Red
    } else {
        Write-Host "Checkboxes found. Toggling state to Checked..." -ForegroundColor DarkCyan
        
        # Toggle each checkbox to Checked
        foreach ($hChk in @($chkReplace, $chkWallpaper, $chkIcons, $chkFallback)) {
            $ctrlId = [DesktopShellTester]::GetDlgCtrlID($hChk)
            [DesktopShellTester]::SendMessageW($hChk, [DesktopShellTester]::BM_SETCHECK, [IntPtr][DesktopShellTester]::BST_CHECKED, [IntPtr]::Zero) | Out-Null
            
            $hDlg = [DesktopShellTester]::GetParent($hChk)
            $wparam = [IntPtr](([DesktopShellTester]::BN_CLICKED -shl 16) -bor $ctrlId)
            [DesktopShellTester]::SendMessageW($hDlg, [DesktopShellTester]::WM_COMMAND, $wparam, $hChk) | Out-Null
        }
        
        Start-Sleep -Milliseconds 500
        
        # Click OK to save and exit dialog
        [DesktopShellTester]::SendMessageW($hPropSheet, [DesktopShellTester]::WM_COMMAND, [IntPtr][DesktopShellTester]::IDOK, [IntPtr]::Zero) | Out-Null
        
        # Wait for settings process to exit
        for ($i = 0; $i -lt 10; $i++) {
            if ($settingsProc.HasExited) { break }
            Start-Sleep -Seconds 1
        }
        
        # Check registry values
        $vReplace = (Get-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled").DesktopReplacementEnabled
        $vWallpaper = (Get-ItemProperty -Path $regPath -Name "DesktopWallpaperEnabled").DesktopWallpaperEnabled
        $vIcons = (Get-ItemProperty -Path $regPath -Name "DesktopIconsEnabled").DesktopIconsEnabled
        $vFallback = (Get-ItemProperty -Path $regPath -Name "FallbackStartMenuEnabled").FallbackStartMenuEnabled
        
        Write-Host "Registry values after dialog exit:" -ForegroundColor Cyan
        Write-Host "  DesktopReplacementEnabled: $vReplace" -ForegroundColor Cyan
        Write-Host "  DesktopWallpaperEnabled:   $vWallpaper" -ForegroundColor Cyan
        Write-Host "  DesktopIconsEnabled:       $vIcons" -ForegroundColor Cyan
        Write-Host "  FallbackStartMenuEnabled:  $vFallback" -ForegroundColor Cyan
        
        if ($vReplace -eq 1 -and $vWallpaper -eq 1 -and $vIcons -eq 1 -and $vFallback -eq 1) {
            $results["SettingsRegistryToggles"] = "PASS"
            Write-Host "[PASS] Settings dialog successfully writes toggles to registry." -ForegroundColor Green
        } else {
            Write-Host "[FAIL] Settings values were not written correctly." -ForegroundColor Red
        }
    }
}
$settingsProc | Stop-Process -Force -ErrorAction SilentlyContinue

# ----------------- TEST 2: Dynamic Taskbar Startup affected by Registry -----------------
Write-Host "`n[TEST 2] Verifying dynamic startup (DesktopReplacementEnabled toggling)..." -ForegroundColor Yellow

# Phase A: DesktopReplacementEnabled = 0
Write-Host "Phase A: Launching with DesktopReplacementEnabled = 0" -ForegroundColor Cyan
Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 0 -Type DWord
Set-ItemProperty -Path $regPath -Name "TaskbarMode" -Value 1 -Type DWord # Replace Mode

$taskbarProc = Start-Process -FilePath ".\EliteTaskbar.exe" -ArgumentList "-allowMultiple" -PassThru
Start-Sleep -Seconds 4

# Check for custom Progman belonging to PID
$customProgmanFound = $false
$hwndProgman = [IntPtr]::Zero
[DesktopShellTester]::EnumWindows([DesktopShellTester+EnumWindowsProc]{
    param($hWnd, $lParam)
    $sbClass = New-Object System.Text.StringBuilder 260
    [DesktopShellTester]::GetClassNameW($hWnd, $sbClass, $sbClass.Capacity) | Out-Null
    if ($sbClass.ToString() -eq "Progman") {
        $pid = 0
        [DesktopShellTester]::GetWindowThreadProcessId($hWnd, [ref]$pid) | Out-Null
        if ($pid -eq $taskbarProc.Id) {
            $script:customProgmanFound = $true
            $script:hwndProgman = $hWnd
        }
    }
    return $true
}, [IntPtr]::Zero) | Out-Null

$taskbarProc | Stop-Process -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

$phaseAPass = $false
if ($customProgmanFound) {
    Write-Host "[FAIL] Custom desktop replacement window was created even though DesktopReplacementEnabled = 0!" -ForegroundColor Red
} else {
    Write-Host "Phase A PASS: Custom desktop replacement window was correctly skipped." -ForegroundColor Green
    $phaseAPass = $true
}

# Phase B: DesktopReplacementEnabled = 1
Write-Host "Phase B: Launching with DesktopReplacementEnabled = 1" -ForegroundColor Cyan
Set-ItemProperty -Path $regPath -Name "DesktopReplacementEnabled" -Value 1 -Type DWord

$taskbarProc = Start-Process -FilePath ".\EliteTaskbar.exe" -ArgumentList "-allowMultiple" -PassThru
Start-Sleep -Seconds 5

$customProgmanFound = $false
$hwndProgman = [IntPtr]::Zero
[DesktopShellTester]::EnumWindows([DesktopShellTester+EnumWindowsProc]{
    param($hWnd, $lParam)
    $sbClass = New-Object System.Text.StringBuilder 260
    [DesktopShellTester]::GetClassNameW($hWnd, $sbClass, $sbClass.Capacity) | Out-Null
    if ($sbClass.ToString() -eq "Progman") {
        $pid = 0
        [DesktopShellTester]::GetWindowThreadProcessId($hWnd, [ref]$pid) | Out-Null
        if ($pid -eq $taskbarProc.Id) {
            $script:customProgmanFound = $true
            $script:hwndProgman = $hWnd
        }
    }
    return $true
}, [IntPtr]::Zero) | Out-Null

$phaseBPass = $false
if ($customProgmanFound -and $hwndProgman -ne [IntPtr]::Zero) {
    Write-Host "Phase B PASS: Custom desktop window successfully created (HWND: $hwndProgman)." -ForegroundColor Green
    $phaseBPass = $true
} else {
    Write-Host "[FAIL] Custom desktop window was not created when active!" -ForegroundColor Red
}

if ($phaseAPass -and $phaseBPass) {
    $results["DesktopStartupDynamic"] = "PASS"
    Write-Host "[PASS] Desktop replacement startup dynamically respects registry toggles." -ForegroundColor Green
}

if ($hwndProgman -eq [IntPtr]::Zero) {
    Write-Host "Desktop window failed to initialize. Skipping remaining desktop checks." -ForegroundColor Red
    $taskbarProc | Stop-Process -Force -ErrorAction SilentlyContinue
    exit 1
}

# ----------------- TEST 3: Class Registration -----------------
Write-Host "`n[TEST 3] Verifying window class hierarchy (Progman -> SHELLDLL_DefView -> SysListView32)..." -ForegroundColor Yellow

$hwndDefView = [DesktopShellTester]::FindWindowExW($hwndProgman, [IntPtr]::Zero, "SHELLDLL_DefView", $null)
$hwndListView = [IntPtr]::Zero
if ($hwndDefView -ne [IntPtr]::Zero) {
    $hwndListView = [DesktopShellTester]::FindWindowExW($hwndDefView, [IntPtr]::Zero, "SysListView32", $null)
}

Write-Host "Window handles found:" -ForegroundColor Cyan
Write-Host "  Progman:          $hwndProgman" -ForegroundColor Cyan
Write-Host "  SHELLDLL_DefView: $hwndDefView" -ForegroundColor Cyan
Write-Host "  SysListView32:    $hwndListView" -ForegroundColor Cyan

if ($hwndDefView -ne [IntPtr]::Zero -and $hwndListView -ne [IntPtr]::Zero) {
    $results["ClassRegistration"] = "PASS"
    Write-Host "[PASS] Custom desktop class registration and parent-child hierarchy verified." -ForegroundColor Green
} else {
    Write-Host "[FAIL] Missing SHELLDLL_DefView or SysListView32 child window." -ForegroundColor Red
}

# ----------------- TEST 4: Z-order Constraints -----------------
Write-Host "`n[TEST 4] Verifying Z-order constraints..." -ForegroundColor Yellow

# Try to bring the Progman window to the top using SetForegroundWindow
Write-Host "Attempting SetForegroundWindow on Progman..." -ForegroundColor Cyan
[DesktopShellTester]::SetForegroundWindow($hwndProgman) | Out-Null
Start-Sleep -Seconds 1
$fgHwnd = [DesktopShellTester]::GetForegroundWindow()
Write-Host "Foreground HWND after focus attempt: $fgHwnd (Progman HWND: $hwndProgman)" -ForegroundColor Cyan

$focusNotStolen = ($fgHwnd -ne $hwndProgman)

# Try to place Progman at top of Z-order via SetWindowPos
Write-Host "Attempting to SetWindowPos(HWND_TOP) on Progman..." -ForegroundColor Cyan
[DesktopShellTester]::SetWindowPos($hwndProgman, [IntPtr]0, 0, 0, 0, 0, 0x0001 -bor 0x0002) | Out-Null # SWP_NOSIZE | SWP_NOMOVE
Start-Sleep -Milliseconds 500

# Traverse window list to check if custom desktop remains at the bottom
$lastHwnd = [DesktopShellTester]::GetWindow($hwndProgman, [DesktopShellTester]::GW_HWNDLAST)
Write-Host "Last HWND in Z-order: $lastHwnd" -ForegroundColor Cyan

$isBottom = ($lastHwnd -eq $hwndProgman -or [DesktopShellTester]::GetWindow($hwndProgman, [DesktopShellTester]::GW_HWNDNEXT) -eq [IntPtr]::Zero)

# Send WM_MOUSEACTIVATE and check if it returns MA_NOACTIVATE (3)
$mouseActRet = [int][DesktopShellTester]::SendMessageW($hwndProgman, 0x0021, $hwndProgman, [IntPtr]0x02010001) # WM_MOUSEACTIVATE, HTCLIENT, WM_LBUTTONDOWN
Write-Host "WM_MOUSEACTIVATE return value: $mouseActRet (Expected: 3 = MA_NOACTIVATE)" -ForegroundColor Cyan

if ($focusNotStolen -and ($mouseActRet -eq 3) -and $isBottom) {
    $results["ZOrderConstraints"] = "PASS"
    Write-Host "[PASS] Custom desktop window successfully locked at the bottom of the Z-order." -ForegroundColor Green
} else {
    Write-Host "[FAIL] Z-order constraint failed: FocusNotStolen=$focusNotStolen, WM_MOUSEACTIVATE=$mouseActRet, IsBottom=$isBottom" -ForegroundColor Red
}

# ----------------- TEST 5: Desktop Icons Population -----------------
Write-Host "`n[TEST 5] Verifying desktop icons population..." -ForegroundColor Yellow

$itemCount = [int][DesktopShellTester]::SendMessageW($hwndListView, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero) # LVM_GETITEMCOUNT
Write-Host "ListView child items found: $itemCount" -ForegroundColor Cyan

# Check actual files on desktop folders
$userDesktop = [System.Environment]::GetFolderPath("Desktop")
$publicDesktop = "C:\Users\Public\Desktop"
$realFileCount = 0
if (Test-Path $userDesktop) { $realFileCount += (Get-ChildItem $userDesktop -Force).Count }
if (Test-Path $publicDesktop) { $realFileCount += (Get-ChildItem $publicDesktop -Force).Count }
Write-Host "Files present in user and public desktop folders: $realFileCount" -ForegroundColor Cyan

if ($itemCount -gt 0) {
    $results["DesktopIconsLoading"] = "PASS"
    Write-Host "[PASS] Desktop items populated successfully into the ListView control." -ForegroundColor Green
} else {
    Write-Host "[FAIL] ListView has 0 items." -ForegroundColor Red
}

# ----------------- TEST 6: SHChangeNotifyRegister & Debounced Refresh -----------------
Write-Host "`n[TEST 6] Verifying directory change monitoring (debounced refresh)..." -ForegroundColor Yellow

$desktopPath = [System.Environment]::GetFolderPath("Desktop")
$tempFilePath = Join-Path $desktopPath "EliteTestIcon.txt"

$countBefore = [int][DesktopShellTester]::SendMessageW($hwndListView, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
Write-Host "Item count before file creation: $countBefore" -ForegroundColor Cyan

# Create file
New-Item -Path $tempFilePath -ItemType File -Force | Out-Null
Write-Host "Created temporary file: $tempFilePath" -ForegroundColor Cyan
# Wait for shell notify and debounced refresh (100ms + buffer)
Start-Sleep -Seconds 2

$countAfterCreate = [int][DesktopShellTester]::SendMessageW($hwndListView, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
Write-Host "Item count after file creation:  $countAfterCreate" -ForegroundColor Cyan

# Delete file
Remove-Item -Path $tempFilePath -Force | Out-Null
Write-Host "Deleted temporary file." -ForegroundColor Cyan
Start-Sleep -Seconds 2

$countAfterDelete = [int][DesktopShellTester]::SendMessageW($hwndListView, 0x1004, [IntPtr]::Zero, [IntPtr]::Zero)
Write-Host "Item count after file deletion:  $countAfterDelete" -ForegroundColor Cyan

$monitored = ($countAfterCreate -gt $countBefore)
$restored = ($countAfterDelete -eq $countBefore)

if ($monitored -and $restored) {
    $results["DirectoryChangeNotify"] = "PASS"
    Write-Host "[PASS] SHChangeNotifyRegister monitors modifications and debounces refresh correctly." -ForegroundColor Green
} else {
    Write-Host "[FAIL] Directory change notification test failed: Monitored=$monitored, Restored=$restored" -ForegroundColor Red
}

# ----------------- TEST 7: Start Button fallback launcher -----------------
Write-Host "`n[TEST 7] Verifying Start Button click triggers Open-Shell fallback launcher (StartMenu.exe)..." -ForegroundColor Yellow

# Find main taskbar window (class Shell_TrayWnd in Replace mode)
$hwndTaskbar = [DesktopShellTester]::FindWindowW("Shell_TrayWnd", $null)
$hwndStartOrb = [DesktopShellTester]::FindWindowExW($hwndTaskbar, [IntPtr]::Zero, "Elite_StartOrbWnd", $null)

Write-Host "Taskbar HWND: $hwndTaskbar | Start Button HWND: $hwndStartOrb" -ForegroundColor Cyan

if ($hwndStartOrb -eq [IntPtr]::Zero) {
    Write-Host "[FAIL] Start button window not found." -ForegroundColor Red
} else {
    # Kill any active StartMenu.exe processes
    Get-Process -Name StartMenu -ErrorAction SilentlyContinue | Stop-Process -Force
    Start-Sleep -Seconds 1
    
    Write-Host "Simulating left-click on the Start Button..." -ForegroundColor Cyan
    # Send WM_LBUTTONDOWN and WM_LBUTTONUP to the Start Orb window
    [DesktopShellTester]::PostMessageW($hwndStartOrb, [DesktopShellTester]::WM_LBUTTONDOWN, [IntPtr][DesktopShellTester]::MK_LBUTTON, [IntPtr]0) | Out-Null
    Start-Sleep -Milliseconds 100
    [DesktopShellTester]::PostMessageW($hwndStartOrb, [DesktopShellTester]::WM_LBUTTONUP, [IntPtr]0, [IntPtr]0) | Out-Null
    
    # Wait for fallback launcher to trigger
    Start-Sleep -Seconds 4
    
    $startMenuProc = Get-Process -Name StartMenu -ErrorAction SilentlyContinue
    if ($null -ne $startMenuProc) {
        $results["StartButtonFallback"] = "PASS"
        Write-Host "[PASS] Start Button click successfully spawned the fallback launcher (PID: $($startMenuProc.Id))." -ForegroundColor Green
        $startMenuProc | Stop-Process -Force
    } else {
        Write-Host "[FAIL] Fallback launcher StartMenu.exe was not spawned." -ForegroundColor Red
    }
}

# Cleanup and stop EliteTaskbar
Write-Host "`n8. Cleaning up EliteTaskbar process..." -ForegroundColor Cyan
$taskbarProc | Stop-Process -Force -ErrorAction SilentlyContinue

# Final Summary Report
Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "  TEST RESULTS SUMMARY" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
$overallPass = $true
foreach ($key in $results.Keys) {
    $res = $results[$key]
    if ($res -eq "PASS") {
        Write-Host "  $key : [PASS]" -ForegroundColor Green
    } else {
        Write-Host "  $key : [FAIL]" -ForegroundColor Red
        $overallPass = $false
    }
}

$verdict = if ($overallPass) { "PASS" } else { "FAIL" }
Write-Host "`nOVERALL VERDICT: $verdict" -ForegroundColor $(if ($overallPass) { "Green" } else { "Red" })

$results | Out-String | Out-File -FilePath "Subagent_Tests\desktop_shell_test_output.txt" -Force
return $verdict
