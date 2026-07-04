#include <windows.h>
#include <commctrl.h>
#include <iostream>

struct TRAYDATA {
    HWND hwnd;
    UINT uID;
    UINT uCallbackMessage;
    DWORD reserved1;
    DWORD reserved2;
    HICON hIcon;
};

int main() {
    HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
    HWND hNotify = FindWindowExW(hTray, NULL, L"TrayNotifyWnd", NULL);
    HWND hSysPager = FindWindowExW(hNotify, NULL, L"SysPager", NULL);
    HWND hToolbar = FindWindowExW(hSysPager, NULL, L"ToolbarWindow32", NULL);
    
    DWORD pid = 0;
    GetWindowThreadProcessId(hToolbar, &pid);
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, pid);
    
    int count = SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0);
    std::cout << "Tray icons: " << count << std::endl;
    
    LPVOID pRemoteTbb = VirtualAllocEx(hProcess, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);
    for (int i = 0; i < count; i++) {
        TBBUTTON tbb = {0};
        SendMessageW(hToolbar, TB_GETBUTTON, i, (LPARAM)pRemoteTbb);
        SIZE_T bytesRead = 0;
        ReadProcessMemory(hProcess, pRemoteTbb, &tbb, sizeof(TBBUTTON), &bytesRead);
        
        TRAYDATA td = {0};
        ReadProcessMemory(hProcess, (LPCVOID)tbb.dwData, &td, sizeof(TRAYDATA), &bytesRead);
        std::cout << "Icon " << i << ": HWND=" << td.hwnd << " MSG=" << td.uCallbackMessage << " ID=" << td.uID << " HICON=" << td.hIcon << std::endl;
    }
    VirtualFreeEx(hProcess, pRemoteTbb, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return 0;
}
