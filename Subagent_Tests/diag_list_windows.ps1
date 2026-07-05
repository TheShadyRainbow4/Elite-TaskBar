# diag_list_windows.ps1
$ErrorActionPreference = 'Stop'

Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$proc = Start-Process -FilePath C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe -ArgumentList "C:\Windows" -PassThru
Start-Sleep -Seconds 5

Add-Type -TypeDefinition @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
}
"@ -ErrorAction SilentlyContinue

$targetPid = $proc.Id
Write-Host "Listing windows for process $targetPid..."

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

Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
