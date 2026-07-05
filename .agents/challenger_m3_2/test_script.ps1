$ErrorActionPreference = 'Stop'
$ScriptDir = "C:\Users\Administrator\Desktop\Elite-TaskBar"
$BinDir = "$ScriptDir\BuildOutput"

# Clean up registry
Remove-Item -Path "HKCU:\Software\Win32Explorer" -Recurse -ErrorAction SilentlyContinue

# Set ViewModeGlobal to 12 (SmallIconTiles) in registry
$regPath = "HKCU:\Software\Win32Explorer\Settings"
New-Item -Path $regPath -Force | Out-Null
Set-ItemProperty -Path $regPath -Name "ViewModeGlobal" -Value 12 -Type DWord

Write-Host "Starting Win32Explorer in SmallIconTiles view mode..." -ForegroundColor Cyan
$p = Start-Process -FilePath "$BinDir\Win32Explorer.exe" -PassThru
Start-Sleep -Seconds 4

Write-Host "Process State: HasExited=$($p.HasExited)" -ForegroundColor Yellow
if ($p.HasExited) {
    Write-Host "Process exited early with code: $($p.ExitCode)" -ForegroundColor Red
}

$hwnd = $p.MainWindowHandle
Write-Host "MainWindowHandle (initial): '$hwnd'" -ForegroundColor Yellow
if ($hwnd -eq 0 -or $hwnd -eq $null) {
    Start-Sleep -Seconds 3
    $p.Refresh()
    $hwnd = $p.MainWindowHandle
    Write-Host "MainWindowHandle (after refresh): '$hwnd'" -ForegroundColor Yellow
}

if ($hwnd -ne 0 -and $hwnd -ne $null) {
    $code = @"
using System;
using System.Runtime.InteropServices;
public class WinUtil {
    [DllImport("user32.dll")]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@
    Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue
    [WinUtil]::SendMessage($hwnd, 0x0010, [IntPtr]0, [IntPtr]0) | Out-Null
    Start-Sleep -Seconds 3
}

if (-not $p.HasExited) {
    Write-Host "Process did not exit automatically. Stopping it..." -ForegroundColor Yellow
    Stop-Process -Id $p.Id -Force
} else {
    Write-Host "Process exited cleanly." -ForegroundColor Green
}

# Verify registry has been updated and ViewModeGlobal is still 12
Write-Host "Checking ViewModeGlobal value..." -ForegroundColor Cyan
$props = Get-ItemProperty -Path $regPath
if ($props.ViewModeGlobal -eq 12) {
    Write-Host "[PASS] ViewModeGlobal remained 12, verifying it is loaded/saved correctly." -ForegroundColor Green
} else {
    Write-Host "[FAIL] ViewModeGlobal value is $($props.ViewModeGlobal) (expected 12)." -ForegroundColor Red
}
