$code = @"
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

public class LockFinder {
    [DllImport("rstrtmgr.dll", CharSet = CharSet.Unicode)]
    public static extern int RmStartSession(out uint pSessionHandle, uint dwSessionFlags, string strSessionKey);

    [DllImport("rstrtmgr.dll")]
    public static extern int RmEndSession(uint dwSession);

    [DllImport("rstrtmgr.dll", CharSet = CharSet.Unicode)]
    public static extern int RmRegisterResources(uint dwSession, uint nFiles, string[] rgsFileNames,
        uint nApplications, uint[] rgApplications, uint nServices, string[] rgsServiceNames);

    [StructLayout(LayoutKind.Sequential)]
    public struct RM_UNIQUE_PROCESS {
        public int dwProcessId;
        public System.Runtime.InteropServices.ComTypes.FILETIME ProcessStartTime;
    }

    public enum RM_APP_TYPE {
        RmUnknownApp = 0,
        RmMainWindow = 1,
        RmOtherWindow = 2,
        RmService = 3,
        RmExplorer = 4,
        RmConsole = 5,
        RmCritical = 1000
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct RM_PROCESS_INFO {
        public RM_UNIQUE_PROCESS Process;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string strAppName;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 64)]
        public string strServiceShortName;
        public RM_APP_TYPE ApplicationType;
        public uint AppStatus;
        public uint TSSessionId;
        [MarshalAs(UnmanagedType.Bool)]
        public bool bGradualRestart;
    }

    [DllImport("rstrtmgr.dll")]
    public static extern int RmGetList(uint dwSession, out uint pnProcInfoNeeded,
        ref uint pnProcInfo, [In, Out] RM_PROCESS_INFO[] rgAffectedApps, out uint lpdwRebootReasons);

    public static List<int> GetLockingProcesses(string path) {
        List<int> pids = new List<int>();
        uint handle;
        string key = Guid.NewGuid().ToString();
        int res = RmStartSession(out handle, 0, key);
        if (res != 0) return pids;

        try {
            string[] files = new string[] { path };
            res = RmRegisterResources(handle, 1, files, 0, null, 0, null);
            if (res != 0) return pids;

            uint needed = 0;
            uint procInfo = 0;
            uint rebootReasons = 0;
            res = RmGetList(handle, out needed, ref procInfo, null, out rebootReasons);
            if (res == 234) { // ERROR_MORE_DATA
                RM_PROCESS_INFO[] apps = new RM_PROCESS_INFO[needed];
                procInfo = needed;
                res = RmGetList(handle, out needed, ref procInfo, apps, out rebootReasons);
                if (res == 0) {
                    for (int i = 0; i < procInfo; i++) {
                        pids.Add(apps[i].Process.dwProcessId);
                    }
                }
            }
        } finally {
            RmEndSession(handle);
        }
        return pids;
    }
}
"@
Add-Type -TypeDefinition $code -ErrorAction SilentlyContinue
$pids = [LockFinder]::GetLockingProcesses("C:\TEMP\elite_taskbar_build.lock")
foreach ($procId in $pids) {
    $proc = Get-Process -Id $procId -ErrorAction SilentlyContinue
    if ($proc) {
        [PSCustomObject]@{
            PID = $procId
            Name = $proc.Name
            Path = $proc.Path
        }
    } else {
        [PSCustomObject]@{
            PID = $procId
            Name = "Unknown"
            Path = "Unknown"
        }
    }
}
