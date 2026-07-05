# diagnose_restart.ps1
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"

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

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessageW(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern IntPtr GetDlgItem(IntPtr hDlg, int nIDDlgItem);
}
"@

Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

# Clean up
Get-Process -Name EliteTaskbar, Win32Explorer, powershell -ErrorAction SilentlyContinue | Where-Object { $_.Id -ne $PID } | Stop-Process -Force
Start-Sleep -Seconds 1

# Start initial taskbar from root
Write-Host "Starting EliteTaskbar from root..." -ForegroundColor Cyan
$procTaskbar = Start-Process -FilePath "$ScriptDir\EliteTaskbar.exe" -PassThru
Start-Sleep -Seconds 2

$initialPid = $procTaskbar.Id
Write-Host "Initial EliteTaskbar PID: $initialPid" -ForegroundColor Yellow

# Launch settings CPL
Write-Host "Launching settings CPL..." -ForegroundColor Cyan
$procCpl = Start-Process -FilePath "control.exe" -ArgumentList "`"$ScriptDir\EliteSettings.cpl`"" -PassThru

# Find settings properties window
$hwndSettings = [IntPtr]::Zero
$settingsPid = 0
for ($i = 0; $i -lt 15; $i++) {
    Start-Sleep -Seconds 1
    $windows = Get-Process | Where-Object { $_.MainWindowTitle -like "*Properties*" }
    foreach ($w in $windows) {
        if ($w.Name -like "EST*") {
            $hwndSettings = $w.MainWindowHandle
            $settingsPid = $w.Id
            break
        }
    }
    if ($hwndSettings -ne [IntPtr]::Zero) { break }
}

if ($hwndSettings -eq [IntPtr]::Zero) {
    Write-Host "Settings window not found!" -ForegroundColor Red
    exit 1
}

Write-Host "Found settings window HWND: $hwndSettings" -ForegroundColor Green

$hApply = [Win32]::GetDlgItem($hwndSettings, 0x3021)
if ($hApply -eq 0) {
    Write-Host "Apply button not found!" -ForegroundColor Red
    exit 1
}

# Click Apply
Write-Host "Clicking Apply..." -ForegroundColor Cyan
[Win32]::SendMessageW($hwndSettings, 0x0111, [IntPtr]0x3021, $hApply) | Out-Null

# Capture powershell command line immediately
Write-Host "Scanning for spawned powershell process..." -ForegroundColor Cyan
$psCmdLine = "Not Found"
for ($i = 0; $i -lt 50; $i++) {
    $psProcs = Get-Process -Name powershell -ErrorAction SilentlyContinue | Where-Object { $_.Id -ne $PID }
    if ($psProcs) {
        $psCmdLine = (Get-CimInstance Win32_Process -Filter "ProcessId = $($psProcs[0].Id)").CommandLine
        break
    }
    Start-Sleep -Milliseconds 50
}

Write-Host "Spawned PowerShell Command Line: $psCmdLine" -ForegroundColor Yellow

# Wait for restart
Write-Host "Waiting for restart..." -ForegroundColor Cyan
$restarted = $false
$newPid = 0
for ($i = 0; $i -lt 15; $i++) {
    Start-Sleep -Seconds 1
    $procs = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue
    if ($procs -and $procs.Id -ne $initialPid) {
        $newPid = $procs.Id
        $restarted = $true
        break
    }
}

if (-not $restarted) {
    Write-Host "EliteTaskbar did not restart!" -ForegroundColor Red
} else {
    Write-Host "EliteTaskbar successfully restarted!" -ForegroundColor Green
    $p = Get-Process -Id $newPid
    $cmd = (Get-CimInstance Win32_Process -Filter "ProcessId = $newPid").CommandLine
    $parent = (Get-CimInstance Win32_Process -Filter "ProcessId = $newPid").ParentProcessId
    $parentName = (Get-Process -Id $parent -ErrorAction SilentlyContinue).Name
    Write-Host "Active EliteTaskbar process PID: $newPid" -ForegroundColor Yellow
    Write-Host "  Path: $($p.Path)" -ForegroundColor Yellow
    Write-Host "  Command Line: $cmd" -ForegroundColor Yellow
    Write-Host "  Parent: $parentName (PID $parent)" -ForegroundColor Yellow
}

# Clean up
Get-Process -Name EliteTaskbar, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Stop-Process -Id $procCpl.Id -Force -ErrorAction SilentlyContinue
