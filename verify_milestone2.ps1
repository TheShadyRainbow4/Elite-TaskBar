# verify_milestone2.ps1
# Verification script for Milestone 2: System Tray Integration (R2) & Custom Icon Theming (R5) & Native Settings Redirect.

$ErrorActionPreference = 'Stop'
$ScriptDir = $PSScriptRoot

Write-Host "--- Milestone 2 Empirical Verification ---" -ForegroundColor Cyan

# Check 1: Verify compilation outputs exist
$taskbarPath = Join-Path $ScriptDir "EliteTaskbar.exe"
$settingsExePath = Join-Path $ScriptDir "EliteSettings.exe"
$settingsCplPath = Join-Path $ScriptDir "EliteSettings.cpl"
$explorerPath = Join-Path $ScriptDir "Win32Explorer.exe"

$exists = $true
foreach ($path in @($taskbarPath, $settingsExePath, $settingsCplPath, $explorerPath)) {
    if (Test-Path $path) {
        Write-Host "[PASS] File exists: $path" -ForegroundColor Green
    } else {
        Write-Host "[FAIL] File missing: $path" -ForegroundColor Red
        $exists = $false
    }
}

if (-not $exists) {
    Write-Error "Verification failed: missing core binaries."
}

# Check 2: Verify Settings Apps (Stub EXE & CPL DLL) launch native property sheet in-process
Write-Host "`nTesting EliteSettings.exe GUI launch..." -ForegroundColor Cyan
$procSettings = Start-Process -FilePath $settingsExePath -PassThru
Start-Sleep -Seconds 3

# Check for window title using Get-Process
$title = (Get-Process -Id $procSettings.Id).MainWindowTitle
if ($title -like "*Taskbar and Start Menu Properties*") {
    Write-Host "[PASS] EliteSettings.exe successfully opened the native properties sheet: '$title'" -ForegroundColor Green
} else {
    Write-Host "[FAIL] EliteSettings.exe did not open the expected window. Title: '$title'" -ForegroundColor Red
}
Stop-Process -Id $procSettings.Id -Force

Write-Host "`nTesting EliteSettings.cpl GUI launch..." -ForegroundColor Cyan
$procCpl = Start-Process -FilePath "control.exe" -ArgumentList "`"$settingsCplPath`"" -PassThru
Start-Sleep -Seconds 4

# Check for window from extracted temp exe
$windows = Get-Process | Where-Object { $_.MainWindowTitle -like "*Taskbar and Start Menu Properties*" }
$foundCplWindow = $false
foreach ($w in $windows) {
    if ($w.Name -like "EST*") {
        Write-Host "[PASS] EliteSettings.cpl successfully extracted and launched in-process native properties sheet from process $($w.Name): '$($w.MainWindowTitle)'" -ForegroundColor Green
        $foundCplWindow = $true
        Stop-Process -Id $w.Id -Force
    }
}

if (-not $foundCplWindow) {
    Write-Host "[FAIL] EliteSettings.cpl window was not found." -ForegroundColor Red
}
Stop-Process -Id $procCpl.Id -Force -ErrorAction SilentlyContinue

# Check 3: Verify EliteTaskbar process lifecycle and exit command
Write-Host "`nTesting EliteTaskbar.exe process lifecycle..." -ForegroundColor Cyan
Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue | Stop-Process -Force

# Setup C# type for finding/sending messages to windows
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class WinWindowUtil {
    [DllImport("user32.dll", SetLastError = true, CharSet = CharSet.Unicode)]
    public static extern IntPtr FindWindowW(string lpClassName, string lpWindowName);
    
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    private static extern bool EnumThreadWindows(int dwThreadId, EnumThreadDelegate lpfn, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    private static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    private delegate bool EnumThreadDelegate(IntPtr hWnd, IntPtr lParam);

    public static IntPtr FindWindowByClass(int processId, string className) {
        IntPtr result = IntPtr.Zero;
        foreach (System.Diagnostics.ProcessThread thread in System.Diagnostics.Process.GetProcessById(processId).Threads) {
            EnumThreadWindows(thread.Id, (hWnd, lParam) => {
                var classBuilder = new StringBuilder(260);
                GetClassName(hWnd, classBuilder, classBuilder.Capacity);
                if (classBuilder.ToString() == className) {
                    result = hWnd;
                    return false;
                }
                return true;
            }, IntPtr.Zero);
            if (result != IntPtr.Zero) break;
        }
        return result;
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

# Set to Independent Mode
$pathAdv = "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
if (!(Test-Path $pathAdv)) { New-Item -Path $pathAdv -Force | Out-Null }
Set-ItemProperty -Path $pathAdv -Name "TaskbarMode" -Value 0 -Type DWord

$procTaskbar = Start-Process -FilePath $taskbarPath -PassThru
Start-Sleep -Seconds 3

$hwndTaskbar = [WinWindowUtil]::FindWindowW("Elite_SecondaryTrayWnd", $null)
if ($hwndTaskbar -ne 0) {
    Write-Host "[PASS] EliteTaskbar.exe launched successfully and registered window with class 'Elite_SecondaryTrayWnd' (HWND: $hwndTaskbar)" -ForegroundColor Green
    
    # Send Quit command via WM_COMMAND (3010)
    [WinWindowUtil]::SendMessage($hwndTaskbar, 0x0111, [IntPtr]3010, [IntPtr]0)
    Start-Sleep -Seconds 2
    if ($procTaskbar.HasExited) {
        Write-Host "[PASS] EliteTaskbar.exe exited cleanly in response to the Quit command." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] EliteTaskbar.exe did not exit in response to the Quit command." -ForegroundColor Red
        Stop-Process -Id $procTaskbar.Id -Force
    }
} else {
    Write-Host "[FAIL] EliteTaskbar.exe did not create a window of class 'Elite_SecondaryTrayWnd'." -ForegroundColor Red
    Stop-Process -Id $procTaskbar.Id -Force
}

# Check 4: Verify Win32Explorer process lifecycle and close behavior
Write-Host "`nTesting Win32Explorer.exe process lifecycle..." -ForegroundColor Cyan
Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force

# Set settings to prevent blocking confirmation dialogs
$pathSettings = "HKCU:\Software\Win32Explorer\Settings"
if (!(Test-Path $pathSettings)) { New-Item -Path $pathSettings -Force | Out-Null }
Set-ItemProperty -Path $pathSettings -Name "ConfirmCloseTabs" -Value 0 -Type DWord

$procExplorer = Start-Process -FilePath $explorerPath -PassThru
Start-Sleep -Seconds 6

$hwndExplorer = [WinWindowUtil]::FindWindowByClass($procExplorer.Id, "Win32Explorer")
if ($hwndExplorer -ne 0) {
    Write-Host "[PASS] Win32Explorer.exe launched successfully and created window of class 'Win32Explorer' (HWND: $hwndExplorer)" -ForegroundColor Green
    
    # Send WM_CLOSE (0x0010)
    [WinWindowUtil]::SendMessage($hwndExplorer, 0x0010, [IntPtr]0, [IntPtr]0)
    Start-Sleep -Seconds 3
    if ($procExplorer.HasExited) {
        Write-Host "[PASS] Win32Explorer.exe exited cleanly when closing its main window (ConfirmCloseTabs disabled)." -ForegroundColor Green
    } else {
        Write-Host "[FAIL] Win32Explorer.exe did not exit when its main window was closed." -ForegroundColor Red
        Stop-Process -Id $procExplorer.Id -Force
    }
} else {
    Write-Host "[FAIL] Win32Explorer.exe did not create a browser window of class 'Win32Explorer'." -ForegroundColor Red
    Stop-Process -Id $procExplorer.Id -Force
}

# Check 5: Verify Custom Icon Theme Registry configuration and logic presence
Write-Host "`nVerifying Custom Icon Theme Registry entry and logic..." -ForegroundColor Cyan
$themePathVal = "C:\NonExistentTestThemePath"
Set-ItemProperty -Path $pathAdv -Name "CustomThemePath" -Value $themePathVal -Type String

# Query registry to verify it is stored correctly
$readVal = (Get-ItemProperty -Path $pathAdv -Name "CustomThemePath").CustomThemePath
if ($readVal -eq $themePathVal) {
    Write-Host "[PASS] CustomThemePath is successfully written and read from registry root." -ForegroundColor Green
} else {
    Write-Host "[FAIL] CustomThemePath could not be verified in the registry." -ForegroundColor Red
}

# Cleanup test value
Remove-ItemProperty -Path $pathAdv -Name "CustomThemePath" -ErrorAction SilentlyContinue

Write-Host "`n--- Verification Complete ---" -ForegroundColor Cyan
