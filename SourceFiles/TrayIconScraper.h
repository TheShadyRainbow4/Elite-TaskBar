#pragma once
#include <windows.h>
#include <vector>

struct ScrapedTrayIcon {
    HWND hwnd;
    UINT uCallbackMessage;
    UINT uID;
    HICON hIcon;
};

std::vector<ScrapedTrayIcon> ScrapeTrayIcons();
void UpdateTrayToolbar(HWND hToolbar, HIMAGELIST hImageList, const std::vector<ScrapedTrayIcon>& icons);
