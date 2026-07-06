# diagnose_settings.ps1
Add-Type -TypeDefinition @"
using System;
using System.Text;
using System.Runtime.InteropServices;

public class Win32 {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    public static void PrintWindowsForProcess(int targetPid) {
        EnumWindows((hWnd, lParam) => {
            uint pid;
            GetWindowThreadProcessId(hWnd, out pid);
            if (pid == targetPid) {
                StringBuilder sbClass = new StringBuilder(260);
                GetClassNameW(hWnd, sbClass, sbClass.Capacity);
                StringBuilder sbText = new StringBuilder(512);
                GetWindowTextW(hWnd, sbText, sbText.Capacity);
                Console.WriteLine("HWND: {0}, Class: {1}, Title: {2}", hWnd.ToInt64().ToString("X"), sbClass.ToString(), sbText.ToString());
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@

Write-Host "Stopping any running EliteSettings..."
Get-Process -Name EliteSettings -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 1

Write-Host "Launching EliteSettings.exe..."
$proc = Start-Process -FilePath "C:\Users\Administrator\Desktop\Elite-TaskBar\EliteSettings.exe" -PassThru
Start-Sleep -Seconds 4

Write-Host "Process HasExited: $($proc.HasExited)"
if ($proc.HasExited) {
    Write-Host "ExitCode: $($proc.ExitCode)"
} else {
    Write-Host "Listing windows for PID $($proc.Id):"
    [Win32]::PrintWindowsForProcess($proc.Id)
    Stop-Process -Id $proc.Id -Force
}
