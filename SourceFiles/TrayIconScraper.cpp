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

#include <map>
#include <string>

std::map<std::pair<HWND, UINT>, std::wstring> g_TrayTooltipsMap;

std::wstring GetScrapedTrayTooltip(HWND hwnd, UINT uID) {
    auto it = g_TrayTooltipsMap.find({hwnd, uID});
    if (it != g_TrayTooltipsMap.end()) {
        return it->second;
    }
    return L"";
}

static HICON GetWindowIconFix(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return NULL;
    DWORD_PTR dwRes = 0;
    if (SendMessageTimeoutW(hwnd, WM_GETICON, 2 /* ICON_SMALL2 */, 0, SMTO_ABORTIFHUNG, 100, &dwRes) && dwRes) {
        return (HICON)dwRes;
    }
    if (SendMessageTimeoutW(hwnd, WM_GETICON, ICON_SMALL, 0, SMTO_ABORTIFHUNG, 100, &dwRes) && dwRes) {
        return (HICON)dwRes;
    }
    if (SendMessageTimeoutW(hwnd, WM_GETICON, ICON_BIG, 0, SMTO_ABORTIFHUNG, 100, &dwRes) && dwRes) {
        return (HICON)dwRes;
    }
    HICON hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICONSM);
    if (hIcon) return hIcon;
    hIcon = (HICON)GetClassLongPtrW(hwnd, GCLP_HICON);
    if (hIcon) return hIcon;
    return NULL;
}

void ScrapeTrayIconsFromToolbar(HWND hToolbar, std::vector<ScrapedTrayIcon>& icons) {
    if (!hToolbar) return;

    DWORD pid = 0;
    GetWindowThreadProcessId(hToolbar, &pid);
    if (!pid) return;

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_OPERATION, FALSE, pid);
    if (!hProcess) return;

    LPVOID pRemoteTbb = VirtualAllocEx(hProcess, NULL, sizeof(TBBUTTON), MEM_COMMIT, PAGE_READWRITE);
    if (!pRemoteTbb) {
        CloseHandle(hProcess);
        return;
    }

    const int maxTipChars = 260;
    LPVOID pRemoteText = VirtualAllocEx(hProcess, NULL, maxTipChars * sizeof(WCHAR), MEM_COMMIT, PAGE_READWRITE);

    int count = (int)SendMessageW(hToolbar, TB_BUTTONCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        TBBUTTON tbb = {0};
        if (SendMessageW(hToolbar, TB_GETBUTTON, i, (LPARAM)pRemoteTbb)) {
            SIZE_T bytesRead = 0;
            ReadProcessMemory(hProcess, pRemoteTbb, &tbb, sizeof(TBBUTTON), &bytesRead);
            if (bytesRead == sizeof(TBBUTTON)) {
                if (tbb.dwData) {
                    BYTE tdBuf[64] = {0};
                    ReadProcessMemory(hProcess, (LPCVOID)tbb.dwData, tdBuf, sizeof(tdBuf), &bytesRead);
                    
                    HWND iconHwnd = *(HWND*)(tdBuf + 0);
                    UINT iconUID = *(UINT*)(tdBuf + 8);
                    UINT iconCallbackMessage = *(UINT*)(tdBuf + 12);
                    HICON iconHIcon = NULL;
                    
                    HICON hIcon10 = *(HICON*)(tdBuf + 24);
                    HICON hIcon11 = *(HICON*)(tdBuf + 16);
                    
                    ICONINFO ii;
                    if (hIcon10 && GetIconInfo(hIcon10, &ii)) {
                        iconHIcon = hIcon10;
                        if (ii.hbmColor) DeleteObject(ii.hbmColor);
                        if (ii.hbmMask) DeleteObject(ii.hbmMask);
                    } else if (hIcon11 && GetIconInfo(hIcon11, &ii)) {
                        iconHIcon = hIcon11;
                        if (ii.hbmColor) DeleteObject(ii.hbmColor);
                        if (ii.hbmMask) DeleteObject(ii.hbmMask);
                    }
                    
                    if (iconHwnd) {
                        ScrapedTrayIcon icon = {0};
                        icon.hwnd = iconHwnd;
                        icon.uCallbackMessage = iconCallbackMessage;
                        icon.uID = iconUID;
                        icon.hIcon = iconHIcon;
                        if (!icon.hIcon) {
                            icon.hIcon = GetWindowIconFix(icon.hwnd);
                        }
                        if (icon.hIcon) {
                            icons.push_back(icon);
                            
                            std::wstring tipText = L"";
                            if (pRemoteText) {
                                int len = (int)SendMessageW(hToolbar, TB_GETBUTTONTEXTW, tbb.idCommand, (LPARAM)pRemoteText);
                                if (len > 0 && len < maxTipChars) {
                                    std::vector<WCHAR> localText(len + 1, 0);
                                    SIZE_T br = 0;
                                    ReadProcessMemory(hProcess, pRemoteText, &localText[0], len * sizeof(WCHAR), &br);
                                    if (br == len * sizeof(WCHAR)) {
                                        tipText = &localText[0];
                                    }
                                }
                            }
                            g_TrayTooltipsMap[{icon.hwnd, icon.uID}] = tipText;
                        }
                    }
                }
            }
        }
    }

    if (pRemoteText) {
        VirtualFreeEx(hProcess, pRemoteText, 0, MEM_RELEASE);
    }
    VirtualFreeEx(hProcess, pRemoteTbb, 0, MEM_RELEASE);
    CloseHandle(hProcess);
}

std::vector<ScrapedTrayIcon> ScrapeTrayIcons() {
    std::vector<ScrapedTrayIcon> icons;
    g_TrayTooltipsMap.clear();

    HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hTray) {
        HWND hNotify = FindWindowExW(hTray, NULL, L"TrayNotifyWnd", NULL);
        if (hNotify) {
            HWND hSysPager = FindWindowExW(hNotify, NULL, L"SysPager", NULL);
            if (hSysPager) {
                HWND hToolbar = FindWindowExW(hSysPager, NULL, L"ToolbarWindow32", NULL);
                ScrapeTrayIconsFromToolbar(hToolbar, icons);
            }
        }
    }

    HWND hOverflow = FindWindowW(L"NotifyIconOverflowWindow", NULL);
    if (hOverflow) {
        HWND hToolbar = FindWindowExW(hOverflow, NULL, L"ToolbarWindow32", NULL);
        ScrapeTrayIconsFromToolbar(hToolbar, icons);
    }

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
