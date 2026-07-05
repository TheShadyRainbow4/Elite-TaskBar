# diag_test.ps1
$ErrorActionPreference = 'Stop'

# Kill existing
Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

# Clear registry
if (Test-Path HKCU:\Software\Win32Explorer) {
    Remove-Item -Path HKCU:\Software\Win32Explorer -Recurse -Force -ErrorAction SilentlyContinue | Out-Null
}

Write-Host "Registry cleared."

# Start process
$proc = Start-Process -FilePath C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe -ArgumentList "C:\Windows" -PassThru
Write-Host "Started process $($proc.Id). Waiting 5 seconds..."
Start-Sleep -Seconds 5

# Send WM_CLOSE
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

$hwnd = [Win32]::FindWindow("Win32Explorer", $null)
if ($hwnd -ne [IntPtr]::Zero) {
    Write-Host "Found window HWND: $hwnd. Sending WM_CLOSE..."
    [Win32]::PostMessage($hwnd, 16, [IntPtr]::Zero, [IntPtr]::Zero) | Out-Null
} else {
    Write-Host "Window not found!"
}

Write-Host "Waiting 5 seconds for exit..."
Start-Sleep -Seconds 5

# Check registry
if (Test-Path HKCU:\Software\Win32Explorer\Settings) {
    Write-Host "Registry settings key exists!"
    Get-ItemProperty -Path HKCU:\Software\Win32Explorer\Settings | Out-String | Write-Host
} else {
    Write-Host "Registry settings key DOES NOT exist!"
}
