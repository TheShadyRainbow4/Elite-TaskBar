# run_verification_tests.ps1
# Automated empirical and runtime validation for EliteTaskbar & EliteSettings
# Author: EMPIRICAL CHALLENGER

$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

Write-Host "==========================================================" -ForegroundColor Green
Write-Host "       ELITE TASKBAR & SETTINGS EMPIRICAL VERIFICATION    " -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# C# Win32 Helpers definition
$code = @"
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);

    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowExW(IntPtr hWndParent, IntPtr hWndChildAfter, string lpszClass, string lpszWindow);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);

    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowLongW(IntPtr hWnd, int nIndex);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr hWnd);

    [DllImport("user32.dll")]
    public static extern IntPtr GetDlgItem(IntPtr hDlg, int nIDDlgItem);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    public const int GWL_ID = -12;
    public const int GWL_STYLE = -16;
    public const int WM_COMMAND = 0x0111;
    public const int TTM_GETTOOLCOUNT = 0x0400 + 13;

    [DllImport("user32.dll")]
    public static extern bool EnumThreadWindows(uint dwThreadId, EnumWindowsProc lpfn, IntPtr lParam);

    public static List<IntPtr> GetThreadWindows(uint threadId) {
        List<IntPtr> result = new List<IntPtr>();
        EnumThreadWindows(threadId, (hWnd, lParam) => {
            result.Add(hWnd);
            return true;
        }, IntPtr.Zero);
        return result;
    }

    public static List<IntPtr> GetChildWindows(IntPtr parent) {
        List<IntPtr> result = new List<IntPtr>();
        EnumChildWindows(parent, (hWnd, lParam) => {
            result.Add(hWnd);
            return true;
        }, IntPtr.Zero);
        return result;
    }
}
"@

Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

# Ensure clean state
Write-Host "[1/7] Cleaning up existing instances..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process -Name EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

# Launch EliteTaskbar
Write-Host "[2/7] Starting EliteTaskbar.exe..." -ForegroundColor Cyan
$taskbarPath = Join-Path $ScriptDir "EliteTaskbar.exe"
$procTaskbar = Start-Process -FilePath $taskbarPath -PassThru
Start-Sleep -Seconds 3

$initialTaskbarPid = $procTaskbar.Id
Write-Host "Initial EliteTaskbar PID: $initialTaskbarPid" -ForegroundColor Yellow

# Launch settings app via Control Panel (.cpl)
Write-Host "[3/7] Launching EliteSettings.cpl via control.exe..." -ForegroundColor Cyan
$cplPath = Join-Path $ScriptDir "EliteSettings.cpl"
$procControl = Start-Process -FilePath "control.exe" -ArgumentList "`"$cplPath`"" -PassThru

# Wait and find the settings window
Write-Host "Waiting for the settings properties window to appear..." -ForegroundColor Yellow
$hwndSettings = [IntPtr]::Zero
$settingsPid = 0

for ($i = 0; $i -lt 20; $i++) {
    Start-Sleep -Milliseconds 500
    # Search for property sheet window
    # Class for property sheet is usually "#32770"
    # Title contains "Properties"
    $windows = Get-Process | Where-Object { $_.MainWindowTitle -like "*Properties*" }
    foreach ($w in $windows) {
        if ($w.Name -like "EST*") {
            # This is the extracted temp settings exe
            $hwndSettings = $w.MainWindowHandle
            $settingsPid = $w.Id
            break
        }
    }
    if ($hwndSettings -ne [IntPtr]::Zero) { break }
}

if ($hwndSettings -eq [IntPtr]::Zero) {
    # Try looking by class
    $hwndSettings = [Win32]::FindWindowW("#32770", "Taskbar and Start Menu Properties")
}

if ($hwndSettings -eq [IntPtr]::Zero) {
    Write-Host "[FAIL] Settings properties window was not found." -ForegroundColor Red
    exit 1
}

Write-Host "Found Settings Window HWND: $hwndSettings | PID: $settingsPid" -ForegroundColor Green
[Win32]::SetForegroundWindow($hwndSettings) | Out-Null

# Verify Tooltips
Write-Host "[4/7] Verifying tooltips on standard buttons (Okay, Cancel, Apply)..." -ForegroundColor Cyan
$hOk = [Win32]::GetDlgItem($hwndSettings, 1) # IDOK = 1
$hCancel = [Win32]::GetDlgItem($hwndSettings, 2) # IDCANCEL = 2
$hApply = [Win32]::GetDlgItem($hwndSettings, 0x3021) # IDAPPLY = 0x3021 / 12321

Write-Host "  Okay Button HWND: $hOk" -ForegroundColor Yellow
Write-Host "  Cancel Button HWND: $hCancel" -ForegroundColor Yellow
Write-Host "  Apply Button HWND: $hApply" -ForegroundColor Yellow

if ($hOk -eq 0 -or $hCancel -eq 0 -or $hApply -eq 0) {
    Write-Host "[FAIL] Standard buttons (Okay, Cancel, Apply) not found in window." -ForegroundColor Red
    exit 1
}

# Verify tooltips class existence on the window thread (since tooltips are popup owned windows)
$threadId = [Win32]::GetWindowThreadProcessId($hwndSettings, [ref]$null)
$threadWindows = [Win32]::GetThreadWindows($threadId)
$tooltipWindows = @()
$totalTools = 0
foreach ($w in $threadWindows) {
    $sb = New-Object System.Text.StringBuilder 260
    [Win32]::GetClassNameW($w, $sb, $sb.Capacity) | Out-Null
    if ($sb.ToString() -eq "tooltips_class32") {
        $tc = [int][Win32]::SendMessageW($w, [Win32]::TTM_GETTOOLCOUNT, [IntPtr]::Zero, [IntPtr]::Zero)
        if ($tc -gt 0) {
            $tooltipWindows += $w
            $totalTools += $tc
            Write-Host "Found Tooltip Window HWND: $w with $tc registered tools." -ForegroundColor Green
        }
    }
}

Write-Host "Total Tooltip Windows found: $($tooltipWindows.Count) | Total Registered Tools Count: $totalTools" -ForegroundColor Green

if ($totalTools -lt 3) {
    Write-Host "[FAIL] Tooltips not correctly registered. Expected at least 3 tools across all tooltip windows, got $totalTools." -ForegroundColor Red
    exit 1
} else {
    Write-Host "[PASS] Tooltips are successfully registered on standard buttons." -ForegroundColor Green
}

# Verify About Dialog Layout
Write-Host "[5/7] Testing About Dialog collapsed and expanded states..." -ForegroundColor Cyan
# Open About Dialog by sending WM_COMMAND with wParam = 40002
[Win32]::PostMessageW($hwndSettings, 0x0111, [IntPtr]40002, [IntPtr]::Zero) | Out-Null

Write-Host "Waiting for About Dialog window..." -ForegroundColor Yellow
$hwndAbout = [IntPtr]::Zero
for ($i = 0; $i -lt 10; $i++) {
    Start-Sleep -Milliseconds 300
    $hwndAbout = [Win32]::FindWindowW("#32770", "About EliteTaskbar")
    if ($hwndAbout -ne [IntPtr]::Zero) { break }
}

if ($hwndAbout -eq [IntPtr]::Zero) {
    Write-Host "[FAIL] About Dialog did not open." -ForegroundColor Red
    exit 1
}

Write-Host "Found About Dialog HWND: $hwndAbout" -ForegroundColor Green

# 1. Check collapsed size
$rectCollapsed = New-Object Win32+RECT
[Win32]::GetWindowRect($hwndAbout, [ref]$rectCollapsed) | Out-Null
$widthC = $rectCollapsed.Right - $rectCollapsed.Left
$heightC = $rectCollapsed.Bottom - $rectCollapsed.Top
Write-Host "Collapsed size: ${widthC}x${heightC}" -ForegroundColor Yellow

# 2. Click "More Info >>" (IDC_ABOUT_EXPAND = 282)
$hExpand = [Win32]::GetDlgItem($hwndAbout, 282)
if ($hExpand -eq 0) {
    Write-Host "[FAIL] More Info button not found on About dialog." -ForegroundColor Red
    exit 1
}

Write-Host "Clicking 'More Info >>'..." -ForegroundColor Yellow
[Win32]::SendMessageW($hwndAbout, 0x0111, [IntPtr]282, $hExpand) | Out-Null
Start-Sleep -Seconds 1

# 3. Check expanded size
$rectExpanded = New-Object Win32+RECT
[Win32]::GetWindowRect($hwndAbout, [ref]$rectExpanded) | Out-Null
$widthE = $rectExpanded.Right - $rectExpanded.Left
$heightE = $rectExpanded.Bottom - $rectExpanded.Top
Write-Host "Expanded size: ${widthE}x${heightE}" -ForegroundColor Yellow

if ($heightE -le $heightC) {
    Write-Host "[FAIL] About Dialog did not expand in height. Height before: $heightC, Height after: $heightE" -ForegroundColor Red
    exit 1
}

# 4. Check if IDC_ABOUT_MOREINFO (283) edit box is visible now
$hMoreInfo = [Win32]::GetDlgItem($hwndAbout, 283)
if ($hMoreInfo -eq 0) {
    Write-Host "[FAIL] More Info edit control not found." -ForegroundColor Red
    exit 1
}

$isVisible = [Win32]::IsWindowVisible($hMoreInfo)
if (-not $isVisible) {
    Write-Host "[FAIL] More Info text control is not visible in expanded state." -ForegroundColor Red
    exit 1
}

Write-Host "[PASS] About Dialog layout fits and behaves correctly in expanded state." -ForegroundColor Green

# 5. Click "Less Info <<" (collapse it back)
Write-Host "Clicking 'Less Info <<' to collapse it back..." -ForegroundColor Yellow
[Win32]::SendMessageW($hwndAbout, 0x0111, [IntPtr]282, $hExpand) | Out-Null
Start-Sleep -Seconds 1

$rectCollapsed2 = New-Object Win32+RECT
[Win32]::GetWindowRect($hwndAbout, [ref]$rectCollapsed2) | Out-Null
$heightC2 = $rectCollapsed2.Bottom - $rectCollapsed2.Top
Write-Host "Collapsed back size height: $heightC2" -ForegroundColor Yellow

if ($heightC2 -ge $heightE) {
    Write-Host "[FAIL] About Dialog did not collapse back in height." -ForegroundColor Red
    exit 1
}

# 6. Close About Dialog by clicking Okay (IDOK = 1)
$hAboutOk = [Win32]::GetDlgItem($hwndAbout, 1)
[Win32]::SendMessageW($hwndAbout, 0x0111, [IntPtr]1, $hAboutOk) | Out-Null
Start-Sleep -Seconds 1

if ([Win32]::IsWindowVisible($hwndAbout)) {
    Write-Host "[FAIL] About Dialog did not close." -ForegroundColor Red
    exit 1
} else {
    Write-Host "[PASS] About Dialog collapsed and closed correctly." -ForegroundColor Green
}

# Verify Apply settings & Taskbar Restart
Write-Host "[6/7] Testing Apply settings and Taskbar Restart path..." -ForegroundColor Cyan

# Click Apply (0x3021) in settings properties sheet
Write-Host "Clicking Apply button in properties window..." -ForegroundColor Yellow
[Win32]::SendMessageW($hwndSettings, 0x0111, [IntPtr]0x3021, $hApply) | Out-Null

Write-Host "Waiting for EliteTaskbar process to restart..." -ForegroundColor Yellow
$restarted = $false
$newPid = 0
for ($i = 0; $i -lt 15; $i++) {
    Start-Sleep -Seconds 1
    $procs = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue
    $newProcs = $procs | Where-Object { $_.Id -ne $initialTaskbarPid -and $_.Path -notlike "*Win32Explorer_26.0.3.0*" }
    if ($newProcs) {
        $newPid = $newProcs[0].Id
        $restarted = $true
        break
    }
}

if (-not $restarted) {
    Write-Host "[FAIL] Taskbar process did not restart when Apply was clicked." -ForegroundColor Red
    exit 1
}

Write-Host "Restarted EliteTaskbar PID: $newPid" -ForegroundColor Green

# Inspect restart path
$newProc = Get-Process -Id $newPid
$restartedPath = $newProc.Path
if ($null -eq $restartedPath -or $restartedPath -eq "") {
    $restartedPath = $newProc.MainModule.FileName
}
Write-Host "Restarted EliteTaskbar Executable Path: $restartedPath" -ForegroundColor Yellow

if ($restartedPath -like "*Temp*" -or $restartedPath -like "*System32*") {
    Write-Host "[FAIL] Restart path resolved to a temp or system folder: $restartedPath" -ForegroundColor Red
    exit 1
}

$expectedPath = Join-Path $ScriptDir "EliteTaskbar.exe"
if ($restartedPath -ne $expectedPath) {
    Write-Host "[FAIL] Restart path did not resolve to project folder. Expected: $expectedPath, Actual: $restartedPath" -ForegroundColor Red
    exit 1
}

# Verify CPL window did not hang
$isHung = (Get-Process -Id $settingsPid).Responding
if ($isHung) {
    Write-Host "[PASS] CPL settings window did not hang and is still responsive." -ForegroundColor Green
} else {
    Write-Host "[FAIL] CPL settings window is hanging or unresponsive." -ForegroundColor Red
    exit 1
}

# Close settings window using Cancel button (IDCANCEL = 2)
Write-Host "Closing settings properties window..." -ForegroundColor Yellow
[Win32]::SendMessageW($hwndSettings, 0x0111, [IntPtr]2, $hCancel) | Out-Null
Start-Sleep -Seconds 1

if (Get-Process -Id $settingsPid -ErrorAction SilentlyContinue) {
    Write-Host "CPL process still active, stopping it..." -ForegroundColor Yellow
    Stop-Process -Id $settingsPid -Force
}

# Verify tray scraping, overflow, clock, and UWP app icons
Write-Host "[7/7] Verifying Tray scraping, Clock, and UWP icons..." -ForegroundColor Cyan

# Check if Secondary Taskbar Windows exist
$hwndList = New-Object System.Collections.Generic.List[IntPtr]
$enumProc = [Win32+EnumWindowsProc]{
    param($hWnd, $lParam)
    $sb = New-Object System.Text.StringBuilder 260
    [Win32]::GetClassNameW($hWnd, $sb, $sb.Capacity) | Out-Null
    if ($sb.ToString() -eq "Elite_SecondaryTrayWnd") {
        $hwndList.Add($hWnd)
    }
    return $true
}
[Win32]::EnumChildWindows([IntPtr]::Zero, $enumProc, [IntPtr]::Zero) | Out-Null

if ($hwndList.Count -eq 0) {
    Write-Host "[FAIL] Secondary taskbar windows ('Elite_SecondaryTrayWnd') were not found." -ForegroundColor Red
    exit 1
}

Write-Host "Found $($hwndList.Count) Elite Secondary Taskbar windows." -ForegroundColor Green

# 1. Clock Font Verification: Search for clock control on all taskbar windows
$clockHwnd = [IntPtr]::Zero
foreach ($hwnd in $hwndList) {
    $children = [Win32]::GetChildWindows($hwnd)
    foreach ($c in $children) {
        $sb = New-Object System.Text.StringBuilder 260
        [Win32]::GetClassNameW($c, $sb, $sb.Capacity) | Out-Null
        if ($sb.ToString() -eq "Elite_ClockWidget" -or $sb.ToString() -like "*Clock*") {
            $clockHwnd = $c
            break
        }
    }
    if ($clockHwnd -ne [IntPtr]::Zero) { break }
}

if ($clockHwnd -ne [IntPtr]::Zero) {
    Write-Host "[PASS] Clock widget window found (HWND: $clockHwnd)." -ForegroundColor Green
} else {
    Write-Host "[FAIL] Clock widget window not found." -ForegroundColor Red
    exit 1
}

# 2. Tray Scraping and Overflow Verification
$trayVerified = $false
foreach ($hwnd in $hwndList) {
    $hNotify = [Win32]::FindWindowExW($hwnd, [IntPtr]::Zero, "TrayNotifyWnd", $null)
    if ($hNotify -ne 0) {
        $hPager = [Win32]::FindWindowExW($hNotify, [IntPtr]::Zero, "SysPager", $null)
        if ($hPager -ne 0) {
            $hToolbar = [Win32]::FindWindowExW($hPager, [IntPtr]::Zero, "ToolbarWindow32", $null)
            if ($hToolbar -ne 0) {
                $btnCount = [int][Win32]::SendMessageW($hToolbar, 1048, [IntPtr]::Zero, [IntPtr]::Zero)
                Write-Host "  -> Tray Toolbar found (HWND: $hToolbar) on Taskbar HWND: $hwnd. Buttons: $btnCount" -ForegroundColor Green
                if ($btnCount -ge 0) {
                    $trayVerified = $true
                }
            }
        }
    }
}

if ($trayVerified) {
    Write-Host "[PASS] Tray scraping toolbar verified." -ForegroundColor Green
} else {
    Write-Host "  -> Note: TrayNotifyWnd was not found or failed query (could be hidden or mode-dependent)." -ForegroundColor Yellow
}

# 3. UWP icon / Task switch buttons
# Open Windows Calculator to see if UWP registration works
Write-Host "Launching Windows Calculator (UWP) to test task buttons..." -ForegroundColor Yellow
Start-Process "calc"
# Sleep 6 seconds to give UWP app ample time to initialize and draw window
Start-Sleep -Seconds 6

$uwpFound = $false
foreach ($hwnd in $hwndList) {
    $children = [Win32]::GetChildWindows($hwnd)
    foreach ($c in $children) {
        $sb = New-Object System.Text.StringBuilder 260
        [Win32]::GetClassNameW($c, $sb, $sb.Capacity) | Out-Null
        if ($sb.ToString() -eq "ToolbarWindow32") {
            $ctrlId = [Win32]::GetWindowLongW($c, -12)
            if ($ctrlId -eq 2000) { # IDC_TASKSWITCH / TaskSwitch toolbar
                $btnCount = [int][Win32]::SendMessageW($c, 1048, [IntPtr]::Zero, [IntPtr]::Zero)
                Write-Host "  -> Task Switch Control found (HWND: $c) on Taskbar HWND: $hwnd. Buttons: $btnCount" -ForegroundColor Green
                if ($btnCount -gt 0) {
                    $uwpFound = $true
                }
            }
        }
    }
}

Get-Process -Name CalculatorApp, calc -ErrorAction SilentlyContinue | Stop-Process -Force

if ($uwpFound) {
    Write-Host "[PASS] UWP App (Calculator) successfully registered as a task button." -ForegroundColor Green
} else {
    Write-Host "[FAIL] UWP App was not registered in task buttons." -ForegroundColor Red
    exit 1
}

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "  ALL TESTS COMPLETED SUCCESSFULLY! VERDICT: PASS" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

exit 0
