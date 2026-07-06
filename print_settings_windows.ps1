$code = @"
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
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue
[Win32]::EnumWindows({
    param($hwnd, $lparam)
    $procId = 0
    [Win32]::GetWindowThreadProcessId($hwnd, [ref]$procId) | Out-Null
    $className = New-Object System.Text.StringBuilder 260
    [Win32]::GetClassNameW($hwnd, $className, $className.Capacity) | Out-Null
    $title = New-Object System.Text.StringBuilder 260
    [Win32]::GetWindowTextW($hwnd, $title, $title.Capacity) | Out-Null
    if ($title.Length -gt 0 -or $className.ToString() -eq "#32770") {
        Write-Host "PID: $procId | HWND: $hwnd | Class: $className | Title: $title"
    }
    return $true
}, [IntPtr]::Zero)
