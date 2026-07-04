#include <windows.h>
#include <commctrl.h>
#include <vector>

struct EliteTrayIcon {
    HWND hwnd;
    UINT uCallbackMessage;
    UINT uID;
    HICON hIcon;
    WCHAR szTip[128];
};

std::vector<EliteTrayIcon> ScrapeTrayIcons() {
    std::vector<EliteTrayIcon> icons;
    HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
    if (!hTray) return icons;
    HWND hNotify = FindWindowExW(hTray, NULL, L"TrayNotifyWnd", NULL);
    if (!hNotify) return icons;
    HWND hSysPager = FindWindowExW(hNotify, NULL, L"SysPager", NULL);
    if (!hSysPager) return icons;
    HWND hToolbar = FindWindowExW(hSysPager, NULL, L"ToolbarWindow32", NULL);
    if (!hToolbar) return icons;

    DWORD pid = 0;
    GetWindowThreadProcessId(hToolbar, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, pid);
    if (!hProcess) return icons;

    LPVOID pRemoteTbb = VirtualAllocEx(hProcess, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteTbb) {
        CloseHandle(hProcess);
        return icons;
    }

    int count = SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        TBBUTTON tbb = {0};
        if (SendMessageW(hToolbar, TB_GETBUTTON, i, (LPARAM)pRemoteTbb)) {
            SIZE_T bytesRead = 0;
            ReadProcessMemory(hProcess, pRemoteTbb, &tbb, sizeof(TBBUTTON), &bytesRead);
            if (bytesRead == sizeof(TBBUTTON)) {
                // In a real implementation, we would extract the extra data (TRAYDATA) attached to tbb.dwData
                // For Windows 7+, TRAYDATA structure needs to be parsed carefully depending on bitness (32 vs 64).
                EliteTrayIcon icon = {0};
                icons.push_back(icon);
            }
        }
    }

    VirtualFreeEx(hProcess, pRemoteTbb, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return icons;
}
