# run_without_deletion.ps1
$ErrorActionPreference = 'Stop'

Get-Process -Name Win32Explorer -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

$explorerPath = "C:\Users\Administrator\Desktop\Elite-TaskBar\Win32Explorer.exe"
$proc = Start-Process -FilePath $explorerPath -ArgumentList "C:\Windows" -PassThru
Start-Sleep -Seconds 4

if ($proc.HasExited) {
    Write-Host "Process exited immediately! ExitCode: $($proc.ExitCode)"
} else {
    Write-Host "Process is successfully running! PID: $($proc.Id)"
    
    # Let's check its window
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
    }
"@ -ErrorAction SilentlyContinue

    [Win32]::EnumWindows(([Win32+EnumWindowsProc]{
        param($hWnd, $lParam)
        $procId = 0
        [Win32]::GetWindowThreadProcessId($hWnd, [ref]$procId) | Out-Null
        if ($procId -eq $proc.Id) {
            $sbClass = New-Object System.Text.StringBuilder 260
            [Win32]::GetClassName($hWnd, $sbClass, $sbClass.Capacity) | Out-Null
            Write-Host "Found Window HWND: $hWnd | Class: $($sbClass.ToString())"
        }
        return $true
    }), [IntPtr]::Zero) | Out-Null

    $proc | Stop-Process -Force
}
