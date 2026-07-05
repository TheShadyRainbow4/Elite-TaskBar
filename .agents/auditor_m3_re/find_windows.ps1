$code = @"
using System;
using System.Text;
using System.Runtime.InteropServices;
public class WinUtil {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassNameW(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowTextW(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    
    public static void PrintWindows() {
        EnumWindows((hWnd, lParam) => {
            StringBuilder sbClass = new StringBuilder(260);
            GetClassNameW(hWnd, sbClass, sbClass.Capacity);
            StringBuilder sbTitle = new StringBuilder(260);
            GetWindowTextW(hWnd, sbTitle, sbTitle.Capacity);
            if (sbClass.ToString().Contains("Elite") || sbClass.ToString().Contains("TrayWnd")) {
                uint pid;
                GetWindowThreadProcessId(hWnd, out pid);
                string procName = "";
                try {
                    procName = System.Diagnostics.Process.GetProcessById((int)pid).ProcessName;
                } catch {}
                Console.WriteLine("HWND: {0} | Class: {1} | Title: {2} | PID: {3} ({4})", hWnd, sbClass.ToString(), sbTitle.ToString(), pid, procName);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue
[WinUtil]::PrintWindows()
