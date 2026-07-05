# run_test_tiles.ps1
$ErrorActionPreference = 'Stop'

Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$regSettingsPath = "HKCU:\Software\Win32Explorer\Settings"
if (Test-Path HKCU:\Software\Win32Explorer) {
    Remove-Item -Path HKCU:\Software\Win32Explorer -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
}

if (!(Test-Path $regSettingsPath)) { New-Item -Path $regSettingsPath -Force | Out-Null }
Set-ItemProperty -Path $regSettingsPath -Name "ViewModeGlobal" -Value 5 -Type DWord # Tiles mode
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
using System.Text;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
}
"@ -ErrorAction SilentlyContinue

Start-Sleep -Seconds 4
Write-Host "Listing all windows for PID $($targetPid):"
[Win32]::EnumWindows(([Win32+EnumWindowsProc]{
    param($hWnd, $lParam)
    $procId = 0
    [Win32]::GetWindowThreadProcessId($hWnd, [ref]$procId) | Out-Null
    if ($procId -eq $targetPid) {
        $sbClass = New-Object System.Text.StringBuilder 260
        [Win32]::GetClassName($hWnd, $sbClass, $sbClass.Capacity) | Out-Null
        $sbTitle = New-Object System.Text.StringBuilder 260
        [Win32]::GetWindowTextW($hWnd, $sbTitle, $sbTitle.Capacity) | Out-Null
        Write-Host "HWND: $hWnd | Class: $($sbClass.ToString()) | Title: $($sbTitle.ToString())"
    }
    return $true
}), [IntPtr]::Zero) | Out-Null

Get-Process -Id $targetPid -ErrorAction SilentlyContinue | Stop-Process -Force
