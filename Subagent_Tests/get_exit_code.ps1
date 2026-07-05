# get_exit_code.ps1
$ErrorActionPreference = 'Stop'

$ScriptDir = Split-Path -Parent -Path $MyInvocation.MyCommand.Definition
$ProjectRoot = Split-Path -Parent -Path $ScriptDir
$explorerExe = Join-Path $ProjectRoot "Win32Explorer.exe"
$xmlPath = Join-Path $ProjectRoot "config.xml"
$regSettingsPath = "HKCU:\Software\Win32Explorer"
$regAdvancedPath = "HKCU:\Software\EliteSoftware\Win32Explorer"

# Kill running instances and clean
cmd /c "taskkill /F /IM Win32Explorer.exe >nul 2>nul"
Start-Sleep -Seconds 1
if (Test-Path $xmlPath) { Remove-Item $xmlPath -Force | Out-Null }
if (Test-Path $regSettingsPath) { Remove-Item -Path $regSettingsPath -Recurse -Force -ErrorAction SilentlyContinue | Out-Null }
if (Test-Path $regAdvancedPath) { Remove-Item -Path $regAdvancedPath -Recurse -Force -ErrorAction SilentlyContinue | Out-Null }

# Pinvoke helper
$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32Debug {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern IntPtr SendMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);

    public static void DumpWindows(int processId) {
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == processId) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassName(hWnd, sbClass, sbClass.Capacity);
                StringBuilder sbTitle = new StringBuilder(260);
                GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
                Console.WriteLine("Handle: 0x{0:X} | PID: {1} | Class: {2} | Title: \"{3}\"", hWnd.ToInt64(), pid, sbClass.ToString(), sbTitle.ToString());
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue

Write-Host "Starting Win32Explorer with clean registry..."
$p = Start-Process -FilePath $explorerExe -ArgumentList "C:\Windows" -PassThru

# Wait for window
$hwndMain = [IntPtr]::Zero
$delegate = [Win32Debug+EnumWindowsProc] {
    param($hWnd, $lParam)
    $sbClass = New-Object System.Text.StringBuilder(260)
    [Win32Debug]::GetClassName($hWnd, $sbClass, $sbClass.Capacity) | Out-Null
    if ($sbClass.ToString() -eq "Win32Explorer") {
        $script:hwndMain = $hWnd
        return $false
    }
    return $true
}

Write-Host "Polling for main window..."
for ($i = 0; $i -lt 30; $i++) {
    [Win32Debug]::EnumWindows($delegate, [IntPtr]::Zero) | Out-Null
    if ($hwndMain -ne [IntPtr]::Zero) { break }
    Start-Sleep -Milliseconds 500
}

if ($hwndMain -eq [IntPtr]::Zero) {
    Write-Host "Main window not found!"
    $p | Stop-Process -Force
    exit 1
}

$explorerPid = 0
[Win32Debug]::GetWindowThreadProcessId($hwndMain, [ref]$explorerPid) | Out-Null
Write-Host "Found main window: 0x$($hwndMain.ToString("X")) for PID $explorerPid"

Write-Host "Sending IDM_TOOLS_OPTIONS (40101)..."
[Win32Debug]::SendMessage($hwndMain, 0x0111, [IntPtr]40101, [IntPtr]::Zero) | Out-Null

Start-Sleep -Seconds 3

# Check if process is still running
$procAfter = Get-Process -Id $explorerPid -ErrorAction SilentlyContinue
if ($null -eq $procAfter) {
    Write-Host "Process $explorerPid CRASHED or exited!" -ForegroundColor Red
} else {
    Write-Host "Process $explorerPid is still running." -ForegroundColor Green
    Write-Host "--- Windows of the process after command ---"
    [Win32Debug]::DumpWindows($explorerPid)
    $p | Stop-Process -Force
}
