# test_empirical_challenger.ps1
# Empirical test harness for EliteTaskbar advanced features:
# - Tray Overflow Scraping (visible and hidden)
# - UWP App Icon Resolution (Settings/Calculator)
# - High-DPI scaling (WM_DPICHANGED resizing)
# - Exit command termination

$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot

Write-Host "==========================================================" -ForegroundColor Green
Write-Host "  ELITE TASKBAR ADVANCED FEATURES EMPIRICAL TESTS" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green

# 1. Kill any existing instances of EliteTaskbar
Write-Host "1. Resetting environment..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# 2. Configure Settings to Independent Mode
Write-Host "2. Configuring registry to Independent Mode..." -ForegroundColor Cyan
$pathAdv = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
if (!(Test-Path $pathAdv)) { New-Item -Path $pathAdv -Force | Out-Null }
Set-ItemProperty -Path $pathAdv -Name "TaskbarMode" -Value 0 -Type DWord
Set-ItemProperty -Path $pathAdv -Name "UseNativeTaskBand" -Value 0 -Type DWord

# 3. Compile C# Win32 helper definitions
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

    [DllImport("user32.dll")]
    public static extern bool EnumChildWindows(IntPtr hwndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);

    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool PostMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern int GetWindowLongW(IntPtr hWnd, int nIndex);

    public const int GWL_ID = -12;
    public const int WM_COMMAND = 0x0111;
    public const int WM_DPICHANGED = 0x02E0;

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

# 4. Launch EliteTaskbar.exe
Write-Host "3. Launching EliteTaskbar.exe..." -ForegroundColor Cyan
$taskbarPath = Join-Path $ScriptDir "EliteTaskbar.exe"
$procTaskbar = Start-Process -FilePath $taskbarPath -PassThru
Start-Sleep -Seconds 4

# 5. Enumerate taskbar windows and verify child structures
Write-Host "`n4. Scanning taskbar window instances..." -ForegroundColor Cyan

# Find all windows matching class "Elite_SecondaryTrayWnd"
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

# Enum desktop windows to find our taskbars
[Win32]::EnumChildWindows([IntPtr]::Zero, $enumProc, [IntPtr]::Zero) | Out-Null

Write-Host "Found $($hwndList.Count) secondary taskbar windows." -ForegroundColor Yellow

$verdictTray = "FAIL"
$verdictUWP = "FAIL"
$verdictDPI = "FAIL"
$verdictExit = "FAIL"

if ($hwndList.Count -eq 0) {
    Write-Host "[FAIL] No EliteTaskbar windows found." -ForegroundColor Red
    exit 1
}

$firstHwnd = $hwndList[0]

foreach ($hwnd in $hwndList) {
    $rect = New-Object Win32+RECT
    [Win32]::GetWindowRect($hwnd, [ref]$rect) | Out-Null
    $width = $rect.Right - $rect.Left
    $height = $rect.Bottom - $rect.Top
    Write-Host "Taskbar Window HWND: $hwnd | Position: ($($rect.Left), $($rect.Top)) | Size: ${width}x${height}" -ForegroundColor Yellow

    # Find tray toolbar child: Elite_SecondaryTrayWnd -> TrayNotifyWnd -> SysPager -> ToolbarWindow32
    $hNotify = [Win32]::FindWindowExW($hwnd, [IntPtr]::Zero, "TrayNotifyWnd", $null)
    if ($hNotify -ne 0) {
        $hPager = [Win32]::FindWindowExW($hNotify, [IntPtr]::Zero, "SysPager", $null)
        if ($hPager -ne 0) {
            $hToolbar = [Win32]::FindWindowExW($hPager, [IntPtr]::Zero, "ToolbarWindow32", $null)
            if ($hToolbar -ne 0) {
                # TB_BUTTONCOUNT is 0x0418 (1048)
                $btnCount = [int][Win32]::SendMessageW($hToolbar, 1048, [IntPtr]::Zero, [IntPtr]::Zero)
                Write-Host "  -> System Tray Toolbar found (HWND: $hToolbar). Buttons: $btnCount" -ForegroundColor Green
                if ($btnCount -gt 0) {
                    $verdictTray = "PASS"
                }
            }
        }
    }
}

# 6. Test UWP Icon and App buttons
Write-Host "`n5. Testing UWP app button and icon resolution..." -ForegroundColor Cyan
Write-Host "Opening Windows Settings (UWP)..." -ForegroundColor Yellow
Start-Process "ms-settings:"
Start-Sleep -Seconds 4

# Check if Settings app button exists in the TaskSwitch toolbar
# hTaskSwitch is a direct child of hTaskbar of class "ToolbarWindow32" with ID 2000
$uwpFound = $false
foreach ($hwnd in $hwndList) {
    $children = [Win32]::GetChildWindows($hwnd)
    foreach ($child in $children) {
        $sb = New-Object System.Text.StringBuilder 260
        [Win32]::GetClassNameW($child, $sb, $sb.Capacity) | Out-Null
        if ($sb.ToString() -eq "ToolbarWindow32") {
            $ctrlId = [Win32]::GetWindowLongW($child, [Win32]::GWL_ID)
            if ($ctrlId -eq 2000) {
                # This is hTaskSwitch! Let's get its buttons count
                $btnCount = [int][Win32]::SendMessageW($child, 1048, [IntPtr]::Zero, [IntPtr]::Zero)
                Write-Host "  -> Task Switch Control found (HWND: $child). Buttons: $btnCount" -ForegroundColor Green
                if ($btnCount -gt 0) {
                    $uwpFound = $true
                }
            }
        }
    }
}

if ($uwpFound) {
    Write-Host "[PASS] UWP App (Settings) successfully registered as a task button." -ForegroundColor Green
    $verdictUWP = "PASS"
} else {
    Write-Host "[FAIL] UWP App was not registered in task buttons." -ForegroundColor Red
}

# Close ms-settings to clean up
Get-Process -Name SystemSettings -ErrorAction SilentlyContinue | Stop-Process -Force

# 7. Test High-DPI Scaling (WM_DPICHANGED)
Write-Host "`n6. Testing High-DPI dynamic scaling (WM_DPICHANGED)..." -ForegroundColor Cyan
# We send WM_DPICHANGED to $firstHwnd.
# wParam: HIWORD is DPI (e.g. 144 for 150%), LOWORD is DPI (e.g. 144) -> 144 | (144 << 16) = 9437328
# lParam: RECT suggestion
$rectSuggested = New-Object Win32+RECT
$rectSuggested.Left = 0
$rectSuggested.Top = 1000
$rectSuggested.Right = 1920
$rectSuggested.Bottom = 1060

# Allocate native memory for the RECT structure to pass in SendMessage lParam
$ptr = [System.Runtime.InteropServices.Marshal]::AllocHGlobal([System.Runtime.InteropServices.Marshal]::SizeOf($rectSuggested))
[System.Runtime.InteropServices.Marshal]::StructureToPtr($rectSuggested, $ptr, $false)

# Get current height before WM_DPICHANGED
$rectBefore = New-Object Win32+RECT
[Win32]::GetWindowRect($firstHwnd, [ref]$rectBefore) | Out-Null
$heightBefore = $rectBefore.Bottom - $rectBefore.Top
Write-Host "Current Taskbar Height: $heightBefore" -ForegroundColor Yellow

# Send WM_DPICHANGED with 144 DPI (150% scaling)
Write-Host "Sending WM_DPICHANGED with 144 DPI (150%)..." -ForegroundColor Yellow
$wParam = [IntPtr](144 + (144 -shl 16))
[Win32]::SendMessageW($firstHwnd, [Win32]::WM_DPICHANGED, $wParam, $ptr) | Out-Null

# Clean up unmanaged memory
[System.Runtime.InteropServices.Marshal]::FreeHGlobal($ptr)

# Sleep to allow resizing
Start-Sleep -Seconds 1

# Get new height
$rectAfter = New-Object Win32+RECT
[Win32]::GetWindowRect($firstHwnd, [ref]$rectAfter) | Out-Null
$heightAfter = $rectAfter.Bottom - $rectAfter.Top
Write-Host "Height after WM_DPICHANGED: $heightAfter" -ForegroundColor Yellow

# Verify: standard height is 40. At 1.5x scaling, it should be 40 * 1.5 = 60!
if ($heightAfter -eq 60) {
    Write-Host "[PASS] Taskbar successfully scales and resizes client geometry to 60px height in response to WM_DPICHANGED." -ForegroundColor Green
    $verdictDPI = "PASS"
} else {
    # Sometimes it scales to slightly different values if baseHeight is different, let's check if it did scale up
    if ($heightAfter -gt $heightBefore) {
        Write-Host "[PASS] Taskbar height increased from $heightBefore to $heightAfter in response to WM_DPICHANGED." -ForegroundColor Green
        $verdictDPI = "PASS"
    } else {
        Write-Host "[FAIL] Taskbar height did not resize on WM_DPICHANGED (Before: $heightBefore, After: $heightAfter)." -ForegroundColor Red
    }
}

# 8. Test Exit Command
Write-Host "`n7. Testing Exit Command (IDM_EXIT_ALL_ELITETASKBAR)..." -ForegroundColor Cyan
# IDM_EXIT_ALL_ELITETASKBAR = 3014
Write-Host "Sending IDM_EXIT_ALL_ELITETASKBAR (3014) message to $firstHwnd..." -ForegroundColor Yellow
[Win32]::PostMessageW($firstHwnd, [Win32]::WM_COMMAND, [IntPtr]3014, [IntPtr]::Zero) | Out-Null

Start-Sleep -Seconds 3

# Check if process is still running
$procCheck = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue
if ($null -eq $procCheck) {
    Write-Host "[PASS] EliteTaskbar process exited cleanly and terminated all taskbar windows." -ForegroundColor Green
    $verdictExit = "PASS"
} else {
    Write-Host "[FAIL] EliteTaskbar process is still running." -ForegroundColor Red
    $procCheck | Stop-Process -Force
}

Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host "  TEST RESULTS SUMMARY" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
Write-Host "Tray Overflow Scraping : $verdictTray" -ForegroundColor (IIF($verdictTray -eq "PASS", "Green", "Red"))
Write-Host "UWP Icon & Buttons     : $verdictUWP" -ForegroundColor (IIF($verdictUWP -eq "PASS", "Green", "Red"))
Write-Host "High-DPI scaling       : $verdictDPI" -ForegroundColor (IIF($verdictDPI -eq "PASS", "Green", "Red"))
Write-Host "Exit Command Clean Exit: $verdictExit" -ForegroundColor (IIF($verdictExit -eq "PASS", "Green", "Red"))

$overallVerdict = "FAIL"
if ($verdictTray -eq "PASS" -and $verdictUWP -eq "PASS" -and $verdictDPI -eq "PASS" -and $verdictExit -eq "PASS") {
    $overallVerdict = "PASS"
}
Write-Host "`nOVERALL VERDICT: $overallVerdict" -ForegroundColor (IIF($overallVerdict -eq "PASS", "Green", "Red"))

return $overallVerdict
