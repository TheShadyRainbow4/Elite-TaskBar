# run_test_close.ps1
$ErrorActionPreference = 'Stop'

Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$regSettingsPath = "HKCU:\Software\Win32Explorer\Settings"
if (Test-Path HKCU:\Software\Win32Explorer) {
    Remove-Item -Path HKCU:\Software\Win32Explorer -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
}

if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 12 -Type DWord
Set-ItemProperty -Path $regSettingsPath -Name "ConfirmCloseTabs" -Value 0 -Type DWord

$explorerPath = "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe"
& psexec64 -i 1 -d $explorerPath "C:\Windows"

# Poll for process
$proc = $null
for ($i = 0; $i -lt 20; $i++) {
    $proc = Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue
    if ($null -ne $proc) { break }
    Start-Sleep -Milliseconds 500
}
if ($null -eq $proc) { throw "Failed to start Win32Explorer." }
$targetPid = $proc.Id
Write-Host "Started PID: $targetPid"

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr FindWindow(string lpClassName, string lpWindowName);
    [DllImport("user32.dll")]
    public static extern bool PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
}
"@ -ErrorAction SilentlyContinue

Start-Sleep -Seconds 4
$hwnd = [Win32]::FindWindow("Win32Explorer", $null)
if ($hwnd -ne [IntPtr]::Zero) {
    Write-Host "Found Window HWND: $hwnd. Sending WM_CLOSE..."
    [Win32]::PostMessage($hwnd, 16, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
} else {
    Write-Host "Window not found!"
}

# Wait for exit
for ($i = 0; $i -lt 20; $i++) {
    $p = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
    if ($null -eq $p) { break }
    Start-Sleep -Milliseconds 500
}

$pCheck = Get-Process -Id $targetPid -ErrorAction SilentlyContinue
if ($null -ne $pCheck) {
    Write-Host "Process did not exit! Force killing..."
    $pCheck | Stop-Process -Force
} else {
    Write-Host "Process exited cleanly."
}

# Check registry
if (Test-Path $regSettingsPath) {
    Write-Host "Registry settings key exists!"
    Get-ItemProperty -Path $regSettingsPath | Select-Object ViewModeGlobal, ConfirmCloseTabs | Out-String | Write-Host
} else {
    Write-Host "Registry settings key DOES NOT exist!"
}
