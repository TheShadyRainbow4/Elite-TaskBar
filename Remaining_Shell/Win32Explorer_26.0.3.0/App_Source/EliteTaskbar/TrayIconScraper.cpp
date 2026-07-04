#include "stdafx.h"
#pragma warning(disable: 4100 4244 4267 4189 4312)
#include <windows.h>
#include <commctrl.h>
#include <vector>
#include "TrayIconScraper.h"

#ifdef _WIN64
struct TRAYDATA {
    HWND hwnd;
    UINT uID;
    UINT uCallbackMessage;
    DWORD reserved1;
    DWORD reserved2;
    HICON hIcon;
};
#else
struct TRAYDATA {
    HWND hwnd;
    UINT uID;
    UINT uCallbackMessage;
    DWORD reserved[2];
    HICON hIcon;
};
#endif

std::vector<ScrapedTrayIcon> g_CurrentTrayIcons;

std::vector<ScrapedTrayIcon> ScrapeTrayIcons() {
    std::vector<ScrapedTrayIcon> icons;
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
                TRAYDATA td = {0};
                if (tbb.dwData) {
                    ReadProcessMemory(hProcess, (LPCVOID)tbb.dwData, &td, sizeof(TRAYDATA), &bytesRead);
                    ScrapedTrayIcon icon = {0};
                    icon.hwnd = td.hwnd;
                    icon.uCallbackMessage = td.uCallbackMessage;
                    icon.uID = td.uID;
                    icon.hIcon = td.hIcon;
                    if (icon.hwnd && icon.hIcon) {
                        icons.push_back(icon);
                    }
                }
            }
        }
    }

    VirtualFreeEx(hProcess, pRemoteTbb, 0, MEM_RELEASE);
    CloseHandle(hProcess);
    return icons;
}

void UpdateTrayToolbar(HWND hToolbar, HIMAGELIST hImageList, const std::vector<ScrapedTrayIcon>& icons) {
    bool changed = false;
    if (icons.size() != g_CurrentTrayIcons.size()) {
        changed = true;
    } else {
        for (size_t i = 0; i < icons.size(); ++i) {
            if (icons[i].hwnd != g_CurrentTrayIcons[i].hwnd || icons[i].uID != g_CurrentTrayIcons[i].uID) {
                changed = true;
                break;
            }
        }
    }
    
    if (changed) {
        g_CurrentTrayIcons = icons;
    }
    
    // Always update the toolbar if it's empty but we have icons, in case this is a new taskbar
    int btnCount = SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0);
    if (!changed && (btnCount > 0 || icons.empty())) return;

    while (SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0) > 0) {
        SendMessageW(hToolbar, TB_DELETEBUTTON, 0, 0);
    }
    ImageList_RemoveAll(hImageList);

    for (size_t i = 0; i < g_CurrentTrayIcons.size(); ++i) {
        int imgIndex = ImageList_AddIcon(hImageList, g_CurrentTrayIcons[i].hIcon);
        TBBUTTON btn = {0};
        btn.iBitmap = imgIndex;
        btn.idCommand = (int)i; // index
        btn.fsState = TBSTATE_ENABLED;
        btn.fsStyle = BTNS_BUTTON;
        SendMessageW(hToolbar, TB_ADDBUTTONS, 1, (LPARAM)&btn);
    }
}



