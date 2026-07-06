#include <windows.h>
#include <restartmanager.h>
#include <iostream>
#include <vector>

#pragma comment(lib, "rstrtmgr.lib")

void PrintLockOwners(const wchar_t* path) {
    DWORD dwSession;
    WCHAR szSessionKey[CCH_RM_SESSION_KEY + 1] = { 0 };
    DWORD dwError = RmStartSession(&dwSession, 0, szSessionKey);
    if (dwError != ERROR_SUCCESS) {
        std::wcout << L"RmStartSession failed with error " << dwError << std::endl;
        return;
    }

    dwError = RmRegisterResources(dwSession, 1, &path, 0, NULL, 0, NULL);
    if (dwError != ERROR_SUCCESS) {
        std::wcout << L"RmRegisterResources failed with error " << dwError << std::endl;
        RmEndSession(dwSession);
        return;
    }

    UINT nProcInfoNeeded = 0;
    UINT nProcInfo = 0;
    DWORD dwRebootReasons = RmRebootReasonNone;
    dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, NULL, &dwRebootReasons);
    if (dwError == ERROR_MORE_DATA) {
        std::vector<RM_PROCESS_INFO> rgAffectedApps(nProcInfoNeeded);
        nProcInfo = nProcInfoNeeded;
        dwError = RmGetList(dwSession, &nProcInfoNeeded, &nProcInfo, &rgAffectedApps[0], &dwRebootReasons);
        if (dwError == ERROR_SUCCESS) {
            std::wcout << L"File " << path << L" is locked by " << nProcInfo << L" processes:" << std::endl;
            for (UINT i = 0; i < nProcInfo; i++) {
                std::wcout << L"  PID: " << rgAffectedApps[i].Process.dwProcessId
                          << L", Name: " << rgAffectedApps[i].strAppName << std::endl;
            }
        } else {
            std::wcout << L"RmGetList failed with error " << dwError << std::endl;
        }
    } else if (dwError == ERROR_SUCCESS) {
        std::wcout << L"File " << path << L" is not locked by any registered processes." << std::endl;
    } else {
        std::wcout << L"RmGetList failed with error " << dwError << std::endl;
    }

    RmEndSession(dwSession);
}

int main() {
    PrintLockOwners(L"C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\Subagent_Tests\\test_theme\\img1.jpg");
    PrintLockOwners(L"C:\\Users\\Administrator\\Desktop\\Elite-TaskBar\\Subagent_Tests\\test_theme\\img2.jpg");
    return 0;
}
