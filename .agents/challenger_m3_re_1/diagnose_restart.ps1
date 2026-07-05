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
Get-Process -Name EliteTaskbar, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# Start initial taskbar from root
Write-Host "Starting EliteTaskbar from root..." -ForegroundColor Cyan
$procTaskbar = Start-Process -FilePath "$ScriptDir\EliteTaskbar.exe" -PassThru
Start-Sleep -Seconds 2

# Launch settings CPL
Write-Host "Launching settings CPL..." -ForegroundColor Cyan
$procCpl = Start-Process -FilePath "control.exe" -ArgumentList "`"$ScriptDir\EliteSettings.cpl`"" -PassThru

# Find settings properties window
Start-Sleep -Seconds 3
$hwndSettings = [Win32]::FindWindowW("#32770", "Taskbar and Start Menu Properties")
if ($hwndSettings -eq 0) {
    Write-Host "Settings window not found!" -ForegroundColor Red
    exit 1
}

$hApply = [Win32]::GetDlgItem($hwndSettings, 0x3021)
if ($hApply -eq 0) {
    Write-Host "Apply button not found!" -ForegroundColor Red
    exit 1
}

# Click Apply
Write-Host "Clicking Apply..." -ForegroundColor Cyan
[Win32]::SendMessageW($hwndSettings, 0x0111, [IntPtr]0x3021, $hApply) | Out-Null

# Wait for restart
Write-Host "Waiting for restart..." -ForegroundColor Cyan
Start-Sleep -Seconds 4

# Check active EliteTaskbar processes
$procs = Get-Process -Name EliteTaskbar -ErrorAction SilentlyContinue
foreach ($p in $procs) {
    $cmd = (Get-CimInstance Win32_Process -Filter "ProcessId = $($p.Id)").CommandLine
    $parent = (Get-CimInstance Win32_Process -Filter "ProcessId = $($p.Id)").ParentProcessId
    $parentName = (Get-Process -Id $parent -ErrorAction SilentlyContinue).Name
    Write-Host "Active EliteTaskbar process PID: $($p.Id)" -ForegroundColor Yellow
    Write-Host "  Path: $($p.Path)" -ForegroundColor Yellow
    Write-Host "  Command Line: $cmd" -ForegroundColor Yellow
    Write-Host "  Parent: $parentName (PID $parent)" -ForegroundColor Yellow
}

# Clean up
Get-Process -Name EliteTaskbar, Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Stop-Process -Id $procCpl.Id -Force -ErrorAction SilentlyContinue
