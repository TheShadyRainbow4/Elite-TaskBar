#include "stdafx.h"
#pragma warning(disable: 4996)
#pragma warning(disable: 4100)
#pragma warning(disable: 4244)
#pragma warning(disable: 4312)
#pragma warning(disable: 4267)
#include "TaskbarWindow.h"
#include "resource.h"
extern HINSTANCE g_hInstance;
#include "Config.h"
#include "Logger.h"
#include "StartButton.h"
#include <commctrl.h>
#include <commdlg.h>
#include <vector>
#include <string>
#include <gdiplus.h>
#include <uxtheme.h>
#include <shlwapi.h>
#include <shlobj.h>
#pragma comment(lib, "comdlg32.lib")

static void AddTooltip(HWND hwndParent, HWND hwndControl, LPCWSTR text) {
    if (!hwndControl) return;
    HWND hwndTooltip = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwndParent, NULL, g_hInstance, NULL);
    if (hwndTooltip) {
        TOOLINFOW ti = { 0 };
        ti.cbSize = sizeof(TOOLINFOW);
        ti.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
        ti.hwnd = hwndParent;
        ti.uId = (UINT_PTR)hwndControl;
        ti.lpszText = const_cast<LPWSTR>(text);
        SendMessageW(hwndTooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
    }
}

static void AddDlgTooltip(HWND hwndDlg, int id, LPCWSTR text) {
    AddTooltip(hwndDlg, GetDlgItem(hwndDlg, id), text);
}

INT_PTR CALLBACK HelpDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        HICON hIcon = LoadIconW(NULL, (LPCWSTR)IDI_QUESTION);
        if (hIcon) {
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessageW(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        AddDlgTooltip(hwndDlg, IDOK, L"Dismiss this help dialog. Go apply your new knowledge!");
        SetDlgItemTextW(hwndDlg, IDC_HELP_TEXT, 
            L"EliteTaskbar Help Documentation\r\n\r\n"
            L"Welcome to EliteTaskbar, the premium legacy taskbar shell extension!\r\n\r\n"
            L"To configure the taskbar, use the tabs on the main properties window:\r\n"
            L"- Taskbar: Adjust the operational mode (Independent vs Replace) and set your custom theme directory path.\r\n"
            L"- Multi-Monitor Components: Enable/disable components like Clock, System Tray, and Task Buttons per monitor.\r\n"
            L"- Native Settings: Configure auto-hide, small icons, locked state, and explorer replacement modes.\r\n"
            L"- Toolbars: Add deskbands and custom folder toolbars.\r\n\r\n"
            L"Need dynamic icons? Just copy your .png/.ico files into the custom theme path and watch them load!\r\n\r\n"
            L"Witty hover tooltips are enabled on every single control to keep you entertained and informed.");
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwndDlg, &ps);
        RECT rcClient;
        GetClientRect(hwndDlg, &rcClient);
        
        RECT rcChin = { 0, 150, 250, 180 };
        MapDialogRect(hwndDlg, &rcChin);
        
        HBRUSH hBrush = CreateSolidBrush(RGB(225, 225, 225));
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 0, rcChin.top, NULL);
        LineTo(hdc, rcClient.right, rcChin.top);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        rcChin.left = 0;
        rcChin.right = rcClient.right;
        rcChin.bottom = rcClient.bottom;
        FillRect(hdc, &rcChin, hBrush);
        DeleteObject(hBrush);
        
        EndPaint(hwndDlg, &ps);
        return TRUE;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        if (pdis->CtlID == IDC_BANNER) {
            FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)GetStockObject(WHITE_BRUSH));
            
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
            HPEN hOldPen = (HPEN)SelectObject(pdis->hDC, hPen);
            MoveToEx(pdis->hDC, 0, pdis->rcItem.bottom - 1, NULL);
            LineTo(pdis->hDC, pdis->rcItem.right, pdis->rcItem.bottom - 1);
            SelectObject(pdis->hDC, hOldPen);
            DeleteObject(hPen);
            
            HICON hIcon = LoadIconW(NULL, (LPCWSTR)IDI_QUESTION);
            if (hIcon) {
                DrawIconEx(pdis->hDC, 10, 5, hIcon, 24, 24, 0, NULL, DI_NORMAL);
            }
            
            HFONT hFont = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Semibold");
            HFONT hOldFont = (HFONT)SelectObject(pdis->hDC, hFont);
            SetBkMode(pdis->hDC, TRANSPARENT);
            SetTextColor(pdis->hDC, RGB(0, 51, 153));
            
            RECT rcText = pdis->rcItem;
            rcText.left += 40;
            rcText.top += 8;
            DrawTextW(pdis->hDC, L"EliteTaskbar Help Topics", -1, &rcText, DT_SINGLELINE | DT_VCENTER);
            
            SelectObject(pdis->hDC, hOldFont);
            DeleteObject(hFont);
            return TRUE;
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowHelpDialog(HWND hwndOwner) {
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_HELP_DIALOG), hwndOwner, HelpDlgProc);
}

INT_PTR CALLBACK AboutDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static bool bExpanded = false;
    switch (uMsg) {
    case WM_INITDIALOG: {
        HICON hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
        if (hIcon) {
            SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessageW(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        }
        bExpanded = false;
        AddDlgTooltip(hwndDlg, IDC_ABOUT_EXPAND, L"Reveal detailed version details, developer credits, and copyright licensing information.");
        AddDlgTooltip(hwndDlg, IDOK, L"Acknowledge this about information dialog and close it.");
        SetDlgItemTextW(hwndDlg, IDC_ABOUT_MOREINFO,
            L"EliteTaskbar Premium Extension Pack\r\n"
            L"Authors: Zachary Whiteman, Susan Gemm, TheShadyRainbow4\r\n"
            L"Company: EliteSoftware / EliteSoftwareTech Co.\r\n"
            L"Document Version: 1.2.0.0 (Four Place Values)\r\n"
            L"Target Framework: .NET Framework 4.6 / Native Win32 C++\r\n"
            L"Minimum OS Target: Windows Vista / Windows 7\r\n\r\n"
            L"This software is protected by international copyright laws. Unauthorized reproduction or distribution is strictly prohibited.");
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwndDlg, &ps);
        RECT rcClient;
        GetClientRect(hwndDlg, &rcClient);
        
        RECT rcChin;
        if (bExpanded) {
            rcChin = { 0, 168, 250, 192 };
        } else {
            rcChin = { 0, 86, 250, 110 };
        }
        MapDialogRect(hwndDlg, &rcChin);
        
        HBRUSH hBrush = CreateSolidBrush(RGB(225, 225, 225));
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, 0, rcChin.top, NULL);
        LineTo(hdc, rcClient.right, rcChin.top);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        rcChin.left = 0;
        rcChin.right = rcClient.right;
        rcChin.bottom = rcClient.bottom;
        FillRect(hdc, &rcChin, hBrush);
        DeleteObject(hBrush);
        
        EndPaint(hwndDlg, &ps);
        return TRUE;
    }
    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pdis = (LPDRAWITEMSTRUCT)lParam;
        if (pdis->CtlID == IDC_BANNER) {
            FillRect(pdis->hDC, &pdis->rcItem, (HBRUSH)GetStockObject(WHITE_BRUSH));
            
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
            HPEN hOldPen = (HPEN)SelectObject(pdis->hDC, hPen);
            MoveToEx(pdis->hDC, 0, pdis->rcItem.bottom - 1, NULL);
            LineTo(pdis->hDC, pdis->rcItem.right, pdis->rcItem.bottom - 1);
            SelectObject(pdis->hDC, hOldPen);
            DeleteObject(hPen);
            
            HICON hIcon = LoadIconW(NULL, (LPCWSTR)IDI_INFORMATION);
            if (hIcon) {
                DrawIconEx(pdis->hDC, 10, 5, hIcon, 24, 24, 0, NULL, DI_NORMAL);
            }
            
            HFONT hFont = CreateFontW(-14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Semibold");
            HFONT hOldFont = (HFONT)SelectObject(pdis->hDC, hFont);
            SetBkMode(pdis->hDC, TRANSPARENT);
            SetTextColor(pdis->hDC, RGB(0, 51, 153));
            
            RECT rcText = pdis->rcItem;
            rcText.left += 40;
            rcText.top += 8;
            DrawTextW(pdis->hDC, L"About EliteTaskbar", -1, &rcText, DT_SINGLELINE | DT_VCENTER);
            
            SelectObject(pdis->hDC, hOldFont);
            DeleteObject(hFont);
            return TRUE;
        }
        break;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;
        }
        if (LOWORD(wParam) == IDC_ABOUT_EXPAND) {
            bExpanded = !bExpanded;
            
            RECT rcWindow, rcClient;
            GetWindowRect(hwndDlg, &rcWindow);
            GetClientRect(hwndDlg, &rcClient);
            int borderX = (rcWindow.right - rcWindow.left) - rcClient.right;
            int borderY = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
            
            if (bExpanded) {
                SetDlgItemTextW(hwndDlg, IDC_ABOUT_EXPAND, L"Less Info <<");
                
                RECT rcDlg = { 0, 0, 250, 192 };
                MapDialogRect(hwndDlg, &rcDlg);
                SetWindowPos(hwndDlg, NULL, 0, 0, rcDlg.right + borderX, rcDlg.bottom + borderY, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                
                ShowWindow(GetDlgItem(hwndDlg, IDC_ABOUT_MOREINFO), SW_SHOW);
                
                RECT rcExpand = { 10, 168, 70, 182 };
                MapDialogRect(hwndDlg, &rcExpand);
                SetWindowPos(GetDlgItem(hwndDlg, IDC_ABOUT_EXPAND), NULL, rcExpand.left, rcExpand.top, rcExpand.right - rcExpand.left, rcExpand.bottom - rcExpand.top, SWP_NOZORDER | SWP_NOACTIVATE);
                
                RECT rcOk = { 190, 168, 240, 182 };
                MapDialogRect(hwndDlg, &rcOk);
                SetWindowPos(GetDlgItem(hwndDlg, IDOK), NULL, rcOk.left, rcOk.top, rcOk.right - rcOk.left, rcOk.bottom - rcOk.top, SWP_NOZORDER | SWP_NOACTIVATE);
            } else {
                SetDlgItemTextW(hwndDlg, IDC_ABOUT_EXPAND, L"More Info >>");
                
                ShowWindow(GetDlgItem(hwndDlg, IDC_ABOUT_MOREINFO), SW_HIDE);
                
                RECT rcExpand = { 10, 90, 70, 104 };
                MapDialogRect(hwndDlg, &rcExpand);
                SetWindowPos(GetDlgItem(hwndDlg, IDC_ABOUT_EXPAND), NULL, rcExpand.left, rcExpand.top, rcExpand.right - rcExpand.left, rcExpand.bottom - rcExpand.top, SWP_NOZORDER | SWP_NOACTIVATE);
                
                RECT rcOk = { 190, 90, 240, 104 };
                MapDialogRect(hwndDlg, &rcOk);
                SetWindowPos(GetDlgItem(hwndDlg, IDOK), NULL, rcOk.left, rcOk.top, rcOk.right - rcOk.left, rcOk.bottom - rcOk.top, SWP_NOZORDER | SWP_NOACTIVATE);
                
                RECT rcDlg = { 0, 0, 250, 110 };
                MapDialogRect(hwndDlg, &rcDlg);
                SetWindowPos(hwndDlg, NULL, 0, 0, rcDlg.right + borderX, rcDlg.bottom + borderY, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
            InvalidateRect(hwndDlg, NULL, TRUE);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

void ShowAboutDialog(HWND hwndOwner) {
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_ABOUT_DIALOG), hwndOwner, AboutDlgProc);
}

LRESULT CALLBACK PropSheetSubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == WM_SHOWWINDOW) {
        static bool bTooltipsAdded = false;
        if (!bTooltipsAdded) {
            bTooltipsAdded = true;
            HWND hOk = GetDlgItem(hWnd, IDOK);
            if (hOk) {
                SetWindowTextW(hOk, L"Okay");
                AddTooltip(hWnd, hOk, L"Commit your preferences, close this window, and hope nothing crashes.");
            }
            HWND hCancel = GetDlgItem(hWnd, IDCANCEL);
            if (hCancel) {
                AddTooltip(hWnd, hCancel, L"Abandon all changes you just made and pretend nothing happened.");
            }
            HWND hApply = GetDlgItem(hWnd, 0x3021);
            if (hApply) {
                AddTooltip(hWnd, hApply, L"Save settings immediately to see if you actually improved anything.");
            }
        }
    }
    if (uMsg == WM_COMMAND) {
        int id = LOWORD(wParam);
        if (id == 40001) {
            ShowHelpDialog(hWnd);
            return 0;
        } else if (id == 40002) {
            ShowAboutDialog(hWnd);
            return 0;
        }
    }
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam) {
    if (uMsg == PSCB_INITIALIZED) {
        HMENU hMenu = CreateMenu();
        HMENU hHelpMenu = CreatePopupMenu();
        AppendMenuW(hHelpMenu, MF_STRING, 40001, L"&Help Topics");
        AppendMenuW(hHelpMenu, MF_STRING, 40002, L"&About EliteTaskbar");
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hHelpMenu, L"&Help");
        SetMenu(hwndDlg, hMenu);
        RECT rc;
        GetWindowRect(hwndDlg, &rc);
        SetWindowPos(hwndDlg, NULL, 0, 0, rc.right - rc.left, (rc.bottom - rc.top) + GetSystemMetrics(SM_CYMENU), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        SetWindowSubclass(hwndDlg, PropSheetSubclassProc, 999, 0);
        
        HWND hOk = GetDlgItem(hwndDlg, IDOK);
        if (hOk) {
            SetWindowTextW(hOk, L"Okay");
            AddTooltip(hwndDlg, hOk, L"Commit your preferences, close this window, and hope nothing crashes.");
        }
        HWND hCancel = GetDlgItem(hwndDlg, IDCANCEL);
        if (hCancel) {
            AddTooltip(hwndDlg, hCancel, L"Abandon all changes you just made and pretend nothing happened.");
        }
        HWND hApply = GetDlgItem(hwndDlg, 0x3021);
        if (hApply) {
            AddTooltip(hwndDlg, hApply, L"Save settings immediately to see if you actually improved anything.");
        }
    }
    return 0;
}

static void BrowseForFolder(HWND hwndOwner, HWND hwndEdit) {
    BROWSEINFOW bi = { 0 };
    bi.hwndOwner = hwndOwner;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI;
    bi.lpszTitle = L"Select Custom Icon Theme Folder:";
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        WCHAR szPath[MAX_PATH];
        if (SHGetPathFromIDListW((PCIDLIST_ABSOLUTE)pidl, szPath)) {
            SetWindowTextW(hwndEdit, szPath);
            SendMessageW(GetParent(hwndEdit), PSM_CHANGED, (WPARAM)GetParent(hwndEdit), 0);
        }
        CoTaskMemFree(pidl);
    }
}

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "uxtheme.lib")

using namespace Gdiplus;

HBITMAP LoadPngResourceAsHBITMAP(DWORD resId) {
    HBITMAP hBitmap = NULL;
    HRSRC hResource = FindResourceW(g_hInstance, MAKEINTRESOURCEW(resId), (LPCWSTR)RT_RCDATA);
    if (!hResource) return NULL;
    DWORD imageSize = SizeofResource(g_hInstance, hResource);
    HGLOBAL hGlobal = LoadResource(g_hInstance, hResource);
    if (!hGlobal) return NULL;
    const void* pResourceData = LockResource(hGlobal);
    if (pResourceData) {
        HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, imageSize);
        if (hBuffer) {
            void* pBuffer = GlobalLock(hBuffer);
            memcpy(pBuffer, pResourceData, imageSize);
            GlobalUnlock(hBuffer);
            IStream* pStream = NULL;
            if (CreateStreamOnHGlobal(hBuffer, TRUE, &pStream) == S_OK) {
                Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
                if (pBitmap && pBitmap->GetLastStatus() == Gdiplus::Ok) {
                    Gdiplus::Color bg(0, 0, 0, 0);
                    UINT width = pBitmap->GetWidth();
                    UINT height = pBitmap->GetHeight();
                    UINT frameHeight = height / 3;
                    Gdiplus::Bitmap* pResized = new Gdiplus::Bitmap(54, 54, PixelFormat32bppARGB);
                    if (pResized) {
                        Gdiplus::Graphics g(pResized);
                        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                        Gdiplus::Rect destRect(0, 0, 54, 54);
                        g.DrawImage(pBitmap, destRect, 0, 0, width, frameHeight, Gdiplus::UnitPixel);
                        pResized->GetHBITMAP(bg, &hBitmap);
                        delete pResized;
                    } else {
                        pBitmap->GetHBITMAP(bg, &hBitmap);
                    }
                }
                if (pBitmap) delete pBitmap;
                pStream->Release();
            }
        }
    }
    return hBitmap;
}

void UpdateOrbPreview(HWND hwndDlg, DWORD orbId) {
    HBITMAP hBitmap = (HBITMAP)LoadImageW(g_hInstance, MAKEINTRESOURCEW(orbId), IMAGE_BITMAP, 54, 54, LR_DEFAULTCOLOR);
    if (hBitmap) {
        HBITMAP hOld = (HBITMAP)SendDlgItemMessageW(hwndDlg, IDC_ORB_PREVIEW, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
        if (hOld) DeleteObject(hOld);
    }
}

void SaveToNativeRegistry(LPCWSTR valueName, DWORD value) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, valueName, 0, REG_DWORD, (const BYTE*)&value, sizeof(DWORD));
        RegCloseKey(hKey);
    }
}

void SetDefaultFileManagerCPP(DWORD mode) {
    HKEY hKey;
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash + 1 - exePath), L"Win32Explorer.exe");
    }

    // Unconditional cleanup to fix being stuck
    SHDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Directory\\shell\\openinWin32Explorer");
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Directory\\shell", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        DWORD cbData = sizeof(WCHAR) * 40;
        WCHAR val[40] = {0};
        if (RegQueryValueExW(hKey, L"", NULL, NULL, (LPBYTE)val, &cbData) == ERROR_SUCCESS) {
            if (wcscmp(val, L"openinWin32Explorer") == 0) {
                RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"none", sizeof(L"none"));
            }
        }
        RegCloseKey(hKey);
    }

    SHDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Folder\\shell\\openinWin32Explorer");
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\Folder\\shell", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        DWORD cbData = sizeof(WCHAR) * 40;
        WCHAR val[40] = {0};
        if (RegQueryValueExW(hKey, L"", NULL, NULL, (LPBYTE)val, &cbData) == ERROR_SUCCESS) {
            if (wcscmp(val, L"openinWin32Explorer") == 0) {
                RegDeleteValueW(hKey, L"");
            }
        }
        RegCloseKey(hKey);
    }

    // Write file manager registry associations based on mode (2=FileSystem, 3=All)
    if (mode == 2 || mode == 3) {
        LPCWSTR rootSubKey = (mode == 2) ? L"Software\\Classes\\Directory\\shell" : L"Software\\Classes\\Folder\\shell";
        WCHAR commandKeyPath[256];
        wsprintfW(commandKeyPath, L"%s\\openinWin32Explorer", rootSubKey);
        
        if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, commandKeyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"Open in Win32Explorer", sizeof(L"Open in Win32Explorer"));
            HKEY hCmdKey;
            if (RegCreateKeyExW(hKey, L"command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hCmdKey, NULL) == ERROR_SUCCESS) {
                WCHAR cmd[MAX_PATH + 10];
                wsprintfW(cmd, L"\"%s\" \"%%1\"", exePath);
                RegSetValueExW(hCmdKey, L"", 0, REG_SZ, (const BYTE*)cmd, (wcslen(cmd) + 1) * sizeof(WCHAR));
                RegCloseKey(hCmdKey);
            }
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, rootSubKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"openinWin32Explorer", sizeof(L"openinWin32Explorer"));
            RegCloseKey(hKey);
        }
    }
}

DWORD WINAPI BroadcastSettingsChangeThread(LPVOID lpParam) {
    // Aggressively restart the apps to ensure all settings are applied safely and cleanly
    WCHAR exePath[MAX_PATH] = {0};
    bool pathResolved = false;
    if (__argc > 1 && __wargv[1] != NULL) {
        DWORD dwAttrs = GetFileAttributesW(__wargv[1]);
        if (dwAttrs != INVALID_FILE_ATTRIBUTES && (dwAttrs & FILE_ATTRIBUTE_DIRECTORY)) {
            wcscpy_s(exePath, MAX_PATH, __wargv[1]);
            pathResolved = true;
        }
    }
    if (!pathResolved) {
        GetModuleFileNameW(g_hInstance, exePath, MAX_PATH);
        PathRemoveFileSpecW(exePath); // Get directory of the current settings CPL/EXE
    }

    wchar_t psCmd[2048];
    swprintf_s(psCmd, L"-NoProfile -WindowStyle Hidden -Command \"Stop-Process -Name EliteTaskbar -Force; Start-Sleep -Milliseconds 500; Start-Process -FilePath '%s\\EliteTaskbar.exe' -ErrorAction SilentlyContinue\"", exePath);

    ShellExecuteW(NULL, NULL, L"powershell.exe", psCmd, NULL, SW_HIDE);

    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
        
    return 0;
}

void SyncAdvancedToMasterAndNative() {
    HKEY hAdvancedKey = NULL;
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hAdvancedKey) != ERROR_SUCCESS) {
        return;
    }
    
    HKEY hMasterKey = NULL;
    RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Master", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hMasterKey, NULL);
    
    DWORD dwIndex = 0;
    wchar_t valueName[16384];
    DWORD cbValueName = 16384;
    DWORD dwType = 0;
    BYTE valData[16384];
    DWORD cbValData = 16384;
    
    while (RegEnumValueW(hAdvancedKey, dwIndex, valueName, &cbValueName, NULL, &dwType, valData, &cbValData) == ERROR_SUCCESS) {
        // 1. Write to Master
        if (hMasterKey) {
            RegSetValueExW(hMasterKey, valueName, 0, dwType, valData, cbValData);
        }
        
        // 2. Sync to native Windows registry paths
        if (_wcsicmp(valueName, L"Hidden") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hNative, L"Hidden", 0, dwType, valData, cbValData);
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"HideFileExt") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hNative, L"HideFileExt", 0, dwType, valData, cbValData);
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"DwmAnimationsEnabled") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\WindowMetrics", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                DWORD val = *(DWORD*)valData;
                const wchar_t* strVal = val ? L"1" : L"0";
                RegSetValueExW(hNative, L"MinAnimate", 0, REG_SZ, (const BYTE*)strVal, (DWORD)(wcslen(strVal) + 1) * sizeof(wchar_t));
                RegCloseKey(hNative);
            }
            ANIMATIONINFO ai = { sizeof(ANIMATIONINFO) };
            ai.iMinAnimate = *(DWORD*)valData;
            SystemParametersInfoW(SPI_SETANIMATION, sizeof(ANIMATIONINFO), &ai, SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
        }
        else if (_wcsicmp(valueName, L"DwmGlassEnabled") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hNative, L"Composition", 0, dwType, valData, cbValData);
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"DwmBorderSize") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop\\WindowMetrics", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                wchar_t strVal[32];
                swprintf_s(strVal, L"%lu", *(DWORD*)valData);
                RegSetValueExW(hNative, L"BorderWidth", 0, REG_SZ, (const BYTE*)strVal, (DWORD)(wcslen(strVal) + 1) * sizeof(wchar_t));
                RegCloseKey(hNative);
            }
        }
        else if (_wcsicmp(valueName, L"DesktopIconsEnabled") == 0) {
            HKEY hNative = NULL;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNative, NULL) == ERROR_SUCCESS) {
                DWORD val = *(DWORD*)valData;
                DWORD hideVal = val ? 0 : 1;
                RegSetValueExW(hNative, L"HideIcons", 0, REG_DWORD, (const BYTE*)&hideVal, sizeof(DWORD));
                RegCloseKey(hNative);
            }
        }
        
        dwIndex++;
        cbValueName = 16384;
        cbValData = 16384;
    }
    
    if (hAdvancedKey) RegCloseKey(hAdvancedKey);
    if (hMasterKey) RegCloseKey(hMasterKey);
}

void NotifySettingsChange() {
    SyncAdvancedToMasterAndNative();

    static ULONGLONG lastTriggerTime = 0;
    ULONGLONG currentTime = GetTickCount64();
    if (currentTime - lastTriggerTime < 1000) {
        return;
    }
    lastTriggerTime = currentTime;

    HANDLE hThread = CreateThread(NULL, 0, BroadcastSettingsChangeThread, NULL, 0, NULL);
    if (hThread) {
        CloseHandle(hThread);
    }
}

INT_PTR CALLBACK TaskbarSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        AddDlgTooltip(hwndDlg, IDC_MODE_INDEPENDENT, L"Run custom taskbars completely separately from Explorer. Twice the taskbars, twice the fun.");
        AddDlgTooltip(hwndDlg, IDC_MODE_REPLACE, L"Replace the native Windows taskbar. Begone, modern Windows shell!");
        AddDlgTooltip(hwndDlg, IDC_MODE_SECONDARY_ONLY, L"Only override secondary monitors. Explorer keeps the primary. Safe compromise?");
        AddDlgTooltip(hwndDlg, IDC_WIDTH_AUTO, L"Automatically adjust button widths based on title length. Enjoy the layout roller coaster.");
        AddDlgTooltip(hwndDlg, IDC_WIDTH_FIXED, L"Enforce a uniform width for all buttons because symmetry is sanity.");
        AddDlgTooltip(hwndDlg, IDC_WIDTH_ICONS, L"Strip the titles entirely. Let's guess what window is what by its icon.");
        AddDlgTooltip(hwndDlg, IDC_WIDTH_FIXED_SIZE, L"Size of taskbar buttons when Fixed width is selected.");
        AddDlgTooltip(hwndDlg, IDC_HOVER_PREVIEW, L"Show window preview thumbnails on hover.");
        AddDlgTooltip(hwndDlg, IDC_PORTABLE_MIRROR, L"Keep settings in a local XML file and HKLM. Portable, like your bad choices.");
        AddDlgTooltip(hwndDlg, IDC_THEME_FOLDER_PATH, L"Directory where your beautiful custom PNG/ICO icons reside. Make it pretty!");
        AddDlgTooltip(hwndDlg, IDC_THEME_FOLDER_BROWSE, L"Open folder selector to find your icon folder. Hope your folders are clean!");
        AddDlgTooltip(hwndDlg, IDC_ENABLE_DARK_MODE, L"Permanently disabled because dark mode is forbidden by our design guidelines!");
        AddDlgTooltip(hwndDlg, IDC_TWO_ROW_TRAY, L"Display the notification area icons in two rows instead of one. Extra space for extra icons.");
        HKEY hKey;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0;
            DWORD cbData = sizeof(DWORD);
            
            if (RegQueryValueExW(hKey, L"TaskbarMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_MODE_INDEPENDENT, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarButtonWidthMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 0) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_AUTO, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_FIXED, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_WIDTH_ICONS, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_WIDTH_AUTO, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            SendMessageW(GetDlgItem(hwndDlg, IDC_WIDTH_FIXED_SIZE), CB_ADDSTRING, 0, (LPARAM)L"Small");
            SendMessageW(GetDlgItem(hwndDlg, IDC_WIDTH_FIXED_SIZE), CB_ADDSTRING, 0, (LPARAM)L"Medium");
            SendMessageW(GetDlgItem(hwndDlg, IDC_WIDTH_FIXED_SIZE), CB_ADDSTRING, 0, (LPARAM)L"Large");
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarButtonFixedWidthSize", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                SendMessageW(GetDlgItem(hwndDlg, IDC_WIDTH_FIXED_SIZE), CB_SETCURSEL, dwValue, 0);
            } else {
                SendMessageW(GetDlgItem(hwndDlg, IDC_WIDTH_FIXED_SIZE), CB_SETCURSEL, 1, 0); // Default to Medium
            }
            
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarHoverPreview", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                SendDlgItemMessageW(hwndDlg, IDC_HOVER_PREVIEW, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
            }

            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"EnablePortableMirror", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                SendDlgItemMessageW(hwndDlg, IDC_PORTABLE_MIRROR, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
            }

            DWORD enableTwoRow = 1;
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"EnableTwoRowTray", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                enableTwoRow = dwValue;
            }
            SendDlgItemMessageW(hwndDlg, IDC_TWO_ROW_TRAY, BM_SETCHECK, enableTwoRow ? BST_CHECKED : BST_UNCHECKED, 0);

            DWORD dwManualTrayWidth = 0;
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"ManualTrayWidth", NULL, NULL, (LPBYTE)&dwManualTrayWidth, &cbData) == ERROR_SUCCESS) {
                SetDlgItemInt(hwndDlg, IDC_MANUAL_TRAY_WIDTH, dwManualTrayWidth, FALSE);
            } else {
                SetDlgItemInt(hwndDlg, IDC_MANUAL_TRAY_WIDTH, 0, FALSE);
            }
            AddDlgTooltip(hwndDlg, IDC_MANUAL_TRAY_WIDTH, L"Enter custom width in pixels for the notification area tray (0 = automatic resizing).");

            WCHAR szThemePath[MAX_PATH] = {0};
            DWORD cbThemePath = sizeof(szThemePath);
            if (RegQueryValueExW(hKey, L"CustomThemePath", NULL, NULL, (LPBYTE)szThemePath, &cbThemePath) == ERROR_SUCCESS) {
                SetDlgItemTextW(hwndDlg, IDC_THEME_FOLDER_PATH, szThemePath);
            }
            EnableWindow(GetDlgItem(hwndDlg, IDC_ENABLE_DARK_MODE), FALSE);
            RegCloseKey(hKey);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_THEME_FOLDER_BROWSE) {
            BrowseForFolder(hwndDlg, GetDlgItem(hwndDlg, IDC_THEME_FOLDER_PATH));
            return TRUE;
        }
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            DWORD portable = (SendDlgItemMessageW(hwndDlg, IDC_PORTABLE_MIRROR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            HKEY hKeyBoth;
            // Write EnablePortableMirror to HKLM and HKCU
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKeyBoth, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hKeyBoth, L"EnablePortableMirror", 0, REG_DWORD, (const BYTE*)&portable, sizeof(DWORD));
                RegCloseKey(hKeyBoth);
            }
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKeyBoth, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hKeyBoth, L"EnablePortableMirror", 0, REG_DWORD, (const BYTE*)&portable, sizeof(DWORD));
                RegCloseKey(hKeyBoth);
            }

            HKEY hKey;
            HKEY hKeyRoot = HKEY_LOCAL_MACHINE;
            if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD mode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_MODE_REPLACE, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_MODE_SECONDARY_ONLY, BM_GETCHECK, 0, 0) == BST_CHECKED) mode = 2;
                RegSetValueExW(hKey, L"TaskbarMode", 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                
                DWORD widthMode = 0;
                if (SendDlgItemMessageW(hwndDlg, IDC_WIDTH_FIXED, BM_GETCHECK, 0, 0) == BST_CHECKED) widthMode = 1;
                else if (SendDlgItemMessageW(hwndDlg, IDC_WIDTH_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) widthMode = 2;
                RegSetValueExW(hKey, L"TaskbarButtonWidthMode", 0, REG_DWORD, (const BYTE*)&widthMode, sizeof(DWORD));
                
                DWORD fixedSize = SendDlgItemMessageW(hwndDlg, IDC_WIDTH_FIXED_SIZE, CB_GETCURSEL, 0, 0);
                if (fixedSize != CB_ERR) {
                    RegSetValueExW(hKey, L"TaskbarButtonFixedWidthSize", 0, REG_DWORD, (const BYTE*)&fixedSize, sizeof(DWORD));
                }
                
                DWORD hoverPreview = (SendDlgItemMessageW(hwndDlg, IDC_HOVER_PREVIEW, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TaskbarHoverPreview", 0, REG_DWORD, (const BYTE*)&hoverPreview, sizeof(DWORD));
                
                DWORD twoRow = (SendDlgItemMessageW(hwndDlg, IDC_TWO_ROW_TRAY, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"EnableTwoRowTray", 0, REG_DWORD, (const BYTE*)&twoRow, sizeof(DWORD));

                DWORD dwManualTrayWidth = GetDlgItemInt(hwndDlg, IDC_MANUAL_TRAY_WIDTH, NULL, FALSE);
                RegSetValueExW(hKey, L"ManualTrayWidth", 0, REG_DWORD, (const BYTE*)&dwManualTrayWidth, sizeof(DWORD));

                WCHAR szThemePath[MAX_PATH] = {0};
                GetDlgItemTextW(hwndDlg, IDC_THEME_FOLDER_PATH, szThemePath, MAX_PATH);
                RegSetValueExW(hKey, L"CustomThemePath", 0, REG_SZ, (const BYTE*)szThemePath, (DWORD)(wcslen(szThemePath) + 1) * sizeof(WCHAR));
                
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK NativeSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        AddDlgTooltip(hwndDlg, IDC_NATIVE_REGISTRY_MODE, L"Sync settings with Windows Explorer. Keep the OS in the loop.");
        AddDlgTooltip(hwndDlg, IDC_USE_NATIVE_TASKBAND, L"Reparent the native taskband for 100% native behaviour. If you dare.");
        AddDlgTooltip(hwndDlg, IDC_TRAY_NATIVE, L"Native Windows 7 style system tray. Nostalgia in its purest form.");
        AddDlgTooltip(hwndDlg, IDC_TRAY_LEGACY, L"Classic linear system tray. Simple, retro, and indestructible.");
        AddDlgTooltip(hwndDlg, IDC_LOCK_TASKBAR, L"Lock the taskbar size and position. Keep it right where you put it.");
        AddDlgTooltip(hwndDlg, IDC_AUTOHIDE_TASKBAR, L"Hide the taskbar when not in use. Out of sight, out of mind.");
        AddDlgTooltip(hwndDlg, IDC_SMALL_ICONS, L"Use smaller taskbar buttons. Great for saving valuable screen real estate.");
        AddDlgTooltip(hwndDlg, IDC_REPLACE_EXPLORER_NONE, L"Do not replace Windows Explorer. Borrrr-ing!");
        AddDlgTooltip(hwndDlg, IDC_REPLACE_EXPLORER_FILESYS, L"Replace file manager for directory paths. Speed up your browsing!");
        AddDlgTooltip(hwndDlg, IDC_REPLACE_EXPLORER_ALL, L"Completely hijack folder browsing. Let Win32Explorer do all the heavy lifting!");
        AddDlgTooltip(hwndDlg, IDC_IMPORT_SETTINGS, L"Import settings from an external registry file. Hopefully from a trusted source...");
        AddDlgTooltip(hwndDlg, IDC_EXPORT_SETTINGS, L"Export settings to a registry file so you can share your questionable design choices.");
        AddDlgTooltip(hwndDlg, IDC_PRIMARY_SHELL_MODE, L"Run EliteTaskbar as primary Windows shell replacement. Control the main desktop lifecycle.");

        HKEY hWinlogon;
        bool isShellReplacement = false;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0, KEY_READ, &hWinlogon) == ERROR_SUCCESS) {
            WCHAR shellVal[MAX_PATH] = {0};
            DWORD cbShellVal = sizeof(shellVal);
            if (RegQueryValueExW(hWinlogon, L"Shell", NULL, NULL, (LPBYTE)shellVal, &cbShellVal) == ERROR_SUCCESS) {
                if (wcsstr(shellVal, L"EliteTaskbar.exe") != NULL) {
                    isShellReplacement = true;
                }
            }
            RegCloseKey(hWinlogon);
        }
        SendDlgItemMessageW(hwndDlg, IDC_PRIMARY_SHELL_MODE, BM_SETCHECK, isShellReplacement ? BST_CHECKED : BST_UNCHECKED, 0);

        HKEY hKey;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0, cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"NativeRegistryMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_NATIVE_REGISTRY_MODE, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
                
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"UseNativeTaskBand", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_USE_NATIVE_TASKBAND, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
                
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TrayMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 1) SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_SETCHECK, BST_CHECKED, 0);
                else SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_TRAY_NATIVE, BM_SETCHECK, BST_CHECKED, 0);
            }

            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"ReplaceExplorerMode", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS) {
                if (dwValue == 2) SendDlgItemMessageW(hwndDlg, IDC_REPLACE_EXPLORER_FILESYS, BM_SETCHECK, BST_CHECKED, 0);
                else if (dwValue == 3) SendDlgItemMessageW(hwndDlg, IDC_REPLACE_EXPLORER_ALL, BM_SETCHECK, BST_CHECKED, 0);
                else SendDlgItemMessageW(hwndDlg, IDC_REPLACE_EXPLORER_NONE, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendDlgItemMessageW(hwndDlg, IDC_REPLACE_EXPLORER_NONE, BM_SETCHECK, BST_CHECKED, 0);
            }
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0, cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_SETCHECK, (dwValue == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
                
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            BYTE blob[64];
            DWORD cbData = sizeof(blob);
            if (RegQueryValueExW(hKey, L"Settings", NULL, NULL, blob, &cbData) == ERROR_SUCCESS && cbData >= 9) {
                bool autoHide = (blob[8] & 0x01) != 0;
                SendDlgItemMessageW(hwndDlg, IDC_AUTOHIDE_TASKBAR, BM_SETCHECK, autoHide ? BST_CHECKED : BST_UNCHECKED, 0);
            }
            RegCloseKey(hKey);
        }
        return TRUE;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        if (wmId == IDC_EXPORT_SETTINGS) {
            WCHAR path[MAX_PATH] = {0};
            OPENFILENAMEW ofn = {0};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwndDlg;
            ofn.lpstrFilter = L"Registry Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt = L"reg";
            if (GetSaveFileNameW(&ofn)) {
                FILE* f;
                _wfopen_s(&f, path, L"w, ccs=UTF-16LE");
                if (f) {
                    fwprintf(f, L"Windows Registry Editor Version 5.00\n");
                    
                    auto WriteKey = [&](HKEY root, LPCWSTR sub, LPCWSTR full) {
                        HKEY hKey;
                        if (RegOpenKeyExW(root, sub, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                            fwprintf(f, L"\n[%s]\n", full);
                            DWORD i = 0; WCHAR name[256]; DWORD nameLen = 256; DWORD type; BYTE data[1024]; DWORD dataLen = 1024;
                            while (RegEnumValueW(hKey, i, name, &nameLen, NULL, &type, data, &dataLen) == ERROR_SUCCESS) {
                                if (type == REG_DWORD && dataLen == 4) { fwprintf(f, L"\"%s\"=dword:%08x\n", name, *(DWORD*)data); }
                                else if (type == REG_SZ) { fwprintf(f, L"\"%s\"=\"%s\"\n", name, (WCHAR*)data); }
                                else if (type == REG_BINARY) {
                                    fwprintf(f, L"\"%s\"=hex:", name);
                                    for (DWORD j = 0; j < dataLen; j++) { fwprintf(f, L"%02x%s", data[j], (j == dataLen - 1) ? L"" : L","); }
                                    fwprintf(f, L"\n");
                                }
                                i++; nameLen = 256; dataLen = 1024;
                            }
                            RegCloseKey(hKey);
                        }
                    };
                    
                    auto WriteVal = [&](HKEY root, LPCWSTR sub, LPCWSTR full, LPCWSTR vname) {
                        HKEY hKey;
                        if (RegOpenKeyExW(root, sub, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                            DWORD type; BYTE data[1024]; DWORD dataLen = 1024;
                            if (RegQueryValueExW(hKey, vname, NULL, &type, data, &dataLen) == ERROR_SUCCESS) {
                                fwprintf(f, L"\n[%s]\n", full);
                                if (type == REG_DWORD && dataLen == 4) { fwprintf(f, L"\"%s\"=dword:%08x\n", vname, *(DWORD*)data); }
                                else if (type == REG_BINARY) {
                                    fwprintf(f, L"\"%s\"=hex:", vname);
                                    for (DWORD j = 0; j < dataLen; j++) { fwprintf(f, L"%02x%s", data[j], (j == dataLen - 1) ? L"" : L","); }
                                    fwprintf(f, L"\n");
                                }
                            }
                            RegCloseKey(hKey);
                        }
                    };

                    WriteKey(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", L"HKEY_LOCAL_MACHINE\\Software\\EliteSoftware\\Win32Explorer\\Advanced");
                    WriteVal(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarSizeMove");
                    WriteVal(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarSmallIcons");
                    WriteVal(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", L"HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", L"Settings");
                    fclose(f);
                    MessageBoxW(hwndDlg, L"Settings exported successfully.", L"Success", MB_ICONINFORMATION);
                }
            }
            return TRUE;
        } else if (wmId == IDC_IMPORT_SETTINGS) {
            WCHAR path[MAX_PATH] = {0};
            OPENFILENAMEW ofn = {0};
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hwndDlg;
            ofn.lpstrFilter = L"Registry Files (*.reg)\0*.reg\0All Files (*.*)\0*.*\0";
            ofn.lpstrFile = path;
            ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST;
            ofn.lpstrDefExt = L"reg";
            if (GetOpenFileNameW(&ofn)) {
                WCHAR cmd[2048];
                swprintf_s(cmd, L"reg import \"%s\"", path);
                _wsystem(cmd);
                
                // Force UI update
                PostMessageW(hwndDlg, WM_INITDIALOG, 0, 0);
                NotifySettingsChange();
                MessageBoxW(hwndDlg, L"Settings imported successfully.", L"Success", MB_ICONINFORMATION);
            }
            return TRUE;
        }
        
        if (HIWORD(wParam) == BN_CLICKED) SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        return TRUE;
    }
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD nativeSync = (SendDlgItemMessageW(hwndDlg, IDC_NATIVE_REGISTRY_MODE, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"NativeRegistryMode", 0, REG_DWORD, (const BYTE*)&nativeSync, sizeof(DWORD));
                
                DWORD useNativeBand = (SendDlgItemMessageW(hwndDlg, IDC_USE_NATIVE_TASKBAND, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"UseNativeTaskBand", 0, REG_DWORD, (const BYTE*)&useNativeBand, sizeof(DWORD));
                
                DWORD trayMode = (SendDlgItemMessageW(hwndDlg, IDC_TRAY_LEGACY, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"TrayMode", 0, REG_DWORD, (const BYTE*)&trayMode, sizeof(DWORD));

                DWORD replaceMode = 1;
                if (SendDlgItemMessageW(hwndDlg, IDC_REPLACE_EXPLORER_FILESYS, BM_GETCHECK, 0, 0) == BST_CHECKED) replaceMode = 2;
                else if (SendDlgItemMessageW(hwndDlg, IDC_REPLACE_EXPLORER_ALL, BM_GETCHECK, 0, 0) == BST_CHECKED) replaceMode = 3;
                RegSetValueExW(hKey, L"ReplaceExplorerMode", 0, REG_DWORD, (const BYTE*)&replaceMode, sizeof(DWORD));
                RegCloseKey(hKey);

                SetDefaultFileManagerCPP(replaceMode);
            }

            HKEY hWinlogon;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon", 0, KEY_SET_VALUE, &hWinlogon) == ERROR_SUCCESS) {
                if (SendDlgItemMessageW(hwndDlg, IDC_PRIMARY_SHELL_MODE, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    WCHAR exePath[MAX_PATH];
                    GetModuleFileNameW(NULL, exePath, MAX_PATH);
                    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
                    if (lastSlash) {
                        wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash + 1 - exePath), L"EliteTaskbar.exe");
                    }
                    RegSetValueExW(hWinlogon, L"Shell", 0, REG_SZ, (const BYTE*)exePath, (DWORD)(wcslen(exePath) + 1) * sizeof(WCHAR));
                } else {
                    RegSetValueExW(hWinlogon, L"Shell", 0, REG_SZ, (const BYTE*)L"explorer.exe", sizeof(L"explorer.exe"));
                }
                RegCloseKey(hWinlogon);
            }
            
            DWORD locked = (SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 0 : 1;
            SaveToNativeRegistry(L"TaskbarSizeMove", locked);
            DWORD smallIcons = (SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            SaveToNativeRegistry(L"TaskbarSmallIcons", smallIcons);
            
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                BYTE blob[64];
                DWORD cbData = sizeof(blob);
                if (RegQueryValueExW(hKey, L"Settings", NULL, NULL, blob, &cbData) == ERROR_SUCCESS && cbData >= 9) {
                    if (SendDlgItemMessageW(hwndDlg, IDC_AUTOHIDE_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) blob[8] |= 0x01;
                    else blob[8] &= ~0x01;
                    RegSetValueExW(hKey, L"Settings", 0, REG_BINARY, blob, cbData);
                }
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

LRESULT CALLBACK DynScrollAreaProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ERASEBKGND:
        case WM_PRINTCLIENT:
            DrawThemeParentBackground(hwnd, (HDC)wParam, NULL);
            return 1;
        case WM_VSCROLL: {
            SCROLLINFO si = { sizeof(si), SIF_ALL };
            GetScrollInfo(hwnd, SB_VERT, &si);
            int oldPos = si.nPos;
            switch (LOWORD(wParam)) {
                case SB_TOP: si.nPos = si.nMin; break;
                case SB_BOTTOM: si.nPos = si.nMax; break;
                case SB_LINEUP: si.nPos -= 20; break;
                case SB_LINEDOWN: si.nPos += 20; break;
                case SB_PAGEUP: si.nPos -= si.nPage; break;
                case SB_PAGEDOWN: si.nPos += si.nPage; break;
                case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
            }
            if (si.nPos < 0) si.nPos = 0;
            if (si.nPos > si.nMax - (int)si.nPage + 1) si.nPos = si.nMax - (int)si.nPage + 1;
            if (si.nPos < 0) si.nPos = 0;
            if (si.nPos != oldPos) {
                ScrollWindowEx(hwnd, 0, oldPos - si.nPos, NULL, NULL, NULL, NULL, SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE);
                SetScrollPos(hwnd, SB_VERT, si.nPos, TRUE);
            }
            return 0;
        }
        case WM_MOUSEWHEEL: {
            int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
            SendMessageW(hwnd, WM_VSCROLL, zDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            SendMessageW(hwnd, WM_VSCROLL, zDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            SendMessageW(hwnd, WM_VSCROLL, zDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
            return 0;
        }
        case WM_COMMAND:
            SendMessageW(GetParent(hwnd), WM_COMMAND, wParam, lParam);
            return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

void InitDynScrollClass() {
    static bool init = false;
    if (init) return;
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = DynScrollAreaProc;
    wc.hInstance = g_hInstance;
    wc.lpszClassName = L"EliteDynScrollArea";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    RegisterClassW(&wc);
    init = true;
}

LRESULT CALLBACK NoMouseWheelSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (uMsg == WM_MOUSEWHEEL) {
        HWND hParent = GetParent(hwnd);
        if (hParent) SendMessageW(hParent, uMsg, wParam, lParam);
        return 0;
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

// Global dynamically populated IDs base
#define ID_BASE_MM_TRAY 10000
#define ID_BASE_MM_CLOCK 11000
#define ID_BASE_MM_TBTN 12000
#define ID_BASE_SM_ORB 13000
#define ID_BASE_SM_TRIG 14000
#define ID_BASE_SM_MODE 15000
#define ID_BASE_SM_PREV 16000

HWND CreateDynScrollArea(HWND hwndDlg, int idc_placeholder) {
    InitDynScrollClass();
    HWND hPlaceholder = GetDlgItem(hwndDlg, idc_placeholder);
    RECT rc = { 0 };
    if (hPlaceholder) {
        GetWindowRect(hPlaceholder, &rc);
        MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc, 2);
    } else {
        GetClientRect(hwndDlg, &rc);
    }
    HWND hScroll = CreateWindowExW(WS_EX_CONTROLPARENT, L"EliteDynScrollArea", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, hwndDlg, NULL, g_hInstance, NULL);
    if (hPlaceholder) DestroyWindow(hPlaceholder);
    return hScroll;
}

// Rename struct to TaskbarPropertiesMonitorInfo to prevent ODR violation - Builder-Bob
struct TaskbarPropertiesMonitorInfo {
    int index;
    HMONITOR hMonitor;
    RECT rect;
};
std::vector<TaskbarPropertiesMonitorInfo> g_Monitors;
BOOL CALLBACK TaskbarPropsMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    TaskbarPropertiesMonitorInfo info;
    info.index = (int)g_Monitors.size();
    info.hMonitor = hMonitor;
    info.rect = *lprcMonitor;
    g_Monitors.push_back(info);
    return TRUE;
}

void PopulateOrbComboBox(HWND hCombo);
void SelectOrbComboBox(HWND hCombo, DWORD id);
DWORD GetSelectedOrbID(HWND hCombo);

INT_PTR CALLBACK MultiMonSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hScroll = NULL;
    static ULONG_PTR gdiplusToken = 0;
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        if (!gdiplusToken) {
            GdiplusStartupInput gdiplusStartupInput;
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        }
        hScroll = CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
        if (g_Monitors.empty()) EnumDisplayMonitors(NULL, NULL, TaskbarPropsMonitorEnumProc, 0);
        
        int y = 5;
        HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
        
        HKEY hKey;
        DWORD migrateStartMenu = 1; // Default to 1
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"MigrateStartMenuSettings", NULL, NULL, (LPBYTE)&migrateStartMenu, &cbData);
            RegCloseKey(hKey);
        }

        RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);
        
        for (const auto& mon : g_Monitors) {
            WCHAR title[64];
            wsprintfW(title, L"Monitor %d (%dx%d)", mon.index, mon.rect.right - mon.rect.left, mon.rect.bottom - mon.rect.top);
            int boxHeight = (migrateStartMenu == 1) ? 60 : 175;
            HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, boxHeight, hScroll, NULL, g_hInstance, NULL);
            SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
            
            HWND hChk1 = CreateWindowExW(0, L"Button", L"System Tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TRAY + mon.index), g_hInstance, NULL);
            HWND hChk2 = CreateWindowExW(0, L"Button", L"Clock", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 120, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_CLOCK + mon.index), g_hInstance, NULL);
            HWND hChk3 = CreateWindowExW(0, L"Button", L"Task Buttons", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 40, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TBTN + mon.index), g_hInstance, NULL);
            
            SendMessageW(hChk1, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk2, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk3, WM_SETFONT, (WPARAM)hFont, 0);
            
            AddTooltip(hScroll, hChk1, L"Show system tray icons on this monitor. Notification overload!");
            AddTooltip(hScroll, hChk2, L"Show clock widget on this monitor. Never lose track of time!");
            AddTooltip(hScroll, hChk3, L"Show application task buttons on this monitor. Keep tabs on everything.");

            if (migrateStartMenu == 0) {
                HWND hLblMode = CreateWindowExW(0, L"Static", L"Start Menu Mode:", WS_CHILD | WS_VISIBLE, 15, y + 65, 120, 15, hScroll, NULL, g_hInstance, NULL);
                HWND hLblTrig = CreateWindowExW(0, L"Static", L"Start Menu Trigger:", WS_CHILD | WS_VISIBLE, 15, y + 90, 120, 15, hScroll, NULL, g_hInstance, NULL);
                HWND hLblOrb = CreateWindowExW(0, L"Static", L"Start Orb Theme:", WS_CHILD | WS_VISIBLE, 15, y + 115, 120, 15, hScroll, NULL, g_hInstance, NULL);

                SendMessageW(hLblMode, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hLblTrig, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hLblOrb, WM_SETFONT, (WPARAM)hFont, 0);

                HWND hCmbMode = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 60, 100, 100, hScroll, (HMENU)(ID_BASE_SM_MODE + mon.index), g_hInstance, NULL);
                HWND hCmbTrig = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 85, 100, 100, hScroll, (HMENU)(ID_BASE_SM_TRIG + mon.index), g_hInstance, NULL);
                HWND hCmbOrb = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 110, 100, 100, hScroll, (HMENU)(ID_BASE_SM_ORB + mon.index), g_hInstance, NULL);
                HWND hPreview = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | WS_BORDER, 250, y + 100, 54, 54, hScroll, (HMENU)(ID_BASE_SM_PREV + mon.index), g_hInstance, NULL);

                AddTooltip(hScroll, hCmbMode, L"Choose which Start Menu to open on this monitor. Make it your own.");
                AddTooltip(hScroll, hCmbTrig, L"Select mouse/keyboard trigger to summon the Start Menu.");
                AddTooltip(hScroll, hCmbOrb, L"Pick the graphic theme for your Start Orb. Show some style!");
                AddTooltip(hScroll, hPreview, L"A preview of your selected Start Orb theme. Looks sharp!");

                SetWindowSubclass(hCmbMode, NoMouseWheelSubclassProc, 1, 0);
                SetWindowSubclass(hCmbTrig, NoMouseWheelSubclassProc, 1, 0);
                SetWindowSubclass(hCmbOrb, NoMouseWheelSubclassProc, 1, 0);

                SendMessageW(hCmbMode, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hCmbTrig, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hCmbOrb, WM_SETFONT, (WPARAM)hFont, 0);

                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Native Injection");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Elite Custom Menu");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Standalone");

                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Left Click");
                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Middle Click");
                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Win Key");

                PopulateOrbComboBox(hCmbOrb);
                
                if (hKey) {
                    DWORD mode = 0, cbData = sizeof(DWORD);
                    WCHAR val[64];
                    wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                    if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&mode, &cbData) != ERROR_SUCCESS) {
                        cbData = sizeof(DWORD);
                        RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&mode, &cbData);
                    }
                    SendMessageW(hCmbMode, CB_SETCURSEL, mode, 0);

                    DWORD trig = 0; cbData = sizeof(DWORD);
                    wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                    if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&trig, &cbData) != ERROR_SUCCESS) {
                        cbData = sizeof(DWORD);
                        RegQueryValueExW(hKey, L"StartMenuTrigger", NULL, NULL, (LPBYTE)&trig, &cbData);
                    }
                    SendMessageW(hCmbTrig, CB_SETCURSEL, trig, 0);

                    DWORD orb = 0; cbData = sizeof(DWORD);
                    wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                    if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&orb, &cbData) == ERROR_SUCCESS) {
                        SelectOrbComboBox(hCmbOrb, orb);
                    } else {
                        SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                        orb = IDB_START_ORB;
                    }
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orb);
                    SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                } else {
                    SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(IDB_START_ORB);
                    SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                    SendMessageW(hCmbMode, CB_SETCURSEL, 0, 0);
                    SendMessageW(hCmbTrig, CB_SETCURSEL, 0, 0);
                }
            }

            if (hKey) {
                DWORD dwValue = 0, cbData = sizeof(DWORD);
                WCHAR val[64];
                wsprintfW(val, L"EnableTray_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) != ERROR_SUCCESS) dwValue = 1;
                if (dwValue) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                
                wsprintfW(val, L"EnableClock_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                else SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                
                wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
                else SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
            }
            
            y += (migrateStartMenu == 1) ? 70 : 185;
        }
        if (hKey) RegCloseKey(hKey);
        
        SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE, 0, y, 200 };
        SetScrollInfo(hScroll, SB_VERT, &si, TRUE);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            
            int id = LOWORD(wParam);
            if (HIWORD(wParam) == CBN_SELCHANGE && id >= ID_BASE_SM_ORB && id < ID_BASE_SM_ORB + 32) {
                int monIndex = id - ID_BASE_SM_ORB;
                HWND hCombo = GetDlgItem(hScroll, id);
                int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    DWORD orbId = SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
                    HWND hPreview = GetDlgItem(hScroll, ID_BASE_SM_PREV + monIndex);
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orbId);
                    HBITMAP hOld = (HBITMAP)SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                    if (hOld) DeleteObject(hOld);
                }
            }
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            DWORD migrateStartMenu = 1; // Default to 1
            if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                DWORD cbData = sizeof(DWORD);
                RegQueryValueExW(hKey, L"MigrateStartMenuSettings", NULL, NULL, (LPBYTE)&migrateStartMenu, &cbData);
                RegCloseKey(hKey);
            }

            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                for (const auto& mon : g_Monitors) {
                    HWND hT = GetDlgItem(hScroll, ID_BASE_MM_TRAY + mon.index);
                    HWND hC = GetDlgItem(hScroll, ID_BASE_MM_CLOCK + mon.index);
                    HWND hB = GetDlgItem(hScroll, ID_BASE_MM_TBTN + mon.index);

                    DWORD v1 = (hT && SendMessageW(hT, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v2 = (hC && SendMessageW(hC, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v3 = (hB && SendMessageW(hB, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    
                    WCHAR val[64];
                    wsprintfW(val, L"EnableTray_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v1, sizeof(DWORD));
                    wsprintfW(val, L"EnableClock_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v2, sizeof(DWORD));
                    wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v3, sizeof(DWORD));
                    
                    if (migrateStartMenu == 0) {
                        HWND hMode = GetDlgItem(hScroll, ID_BASE_SM_MODE + mon.index);
                        HWND hTrig = GetDlgItem(hScroll, ID_BASE_SM_TRIG + mon.index);
                        HWND hOrb = GetDlgItem(hScroll, ID_BASE_SM_ORB + mon.index);

                        DWORD mode = hMode ? SendMessageW(hMode, CB_GETCURSEL, 0, 0) : 0;
                        DWORD trig = hTrig ? SendMessageW(hTrig, CB_GETCURSEL, 0, 0) : 0;
                        DWORD orb = hOrb ? GetSelectedOrbID(hOrb) : IDB_START_ORB;

                        wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                        wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                        wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&orb, sizeof(DWORD));
                    }
                }
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

void PopulateOrbComboBox(HWND hCombo) {
    struct OrbItem { LPCWSTR name; DWORD id; };
    OrbItem items[] = {
        { L"Classic Orb", IDB_START_ORB },
        { L"1Orb", IDB_START_ORB_1ORB },
        { L"Aqua Bottom", IDB_START_ORB_AQUABOTTOM },
        { L"Dunes", IDB_START_ORB_DUNES },
        { L"Indigo", IDB_START_ORB_INDIGO },
        { L"Sapphire", IDB_START_ORB_SAPPHIRE },
        { L"Uranus", IDB_START_ORB_URANUS },
        { L"Vienna Bottom", IDB_START_ORB_VIENNABOTTOM }
    };
    for (int i = 0; i < sizeof(items)/sizeof(items[0]); i++) {
        int idx = SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)items[i].name);
        SendMessageW(hCombo, CB_SETITEMDATA, idx, items[i].id);
    }
}

void PopulateTriggerComboBox(HWND hCombo) {
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Native Injection");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Elite Custom Menu");
}


INT_PTR CALLBACK ToolbarsSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG:
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Address");
        SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Links");
        SendDlgItemMessageW(hwndDlg, IDC_TOOLBAR_LIST, LB_ADDSTRING, 0, (LPARAM)L"Desktop");
        AddDlgTooltip(hwndDlg, IDC_TOOLBAR_LIST, L"List of active deskband toolbars. Toggle them to clutter your taskbar.");
        AddDlgTooltip(hwndDlg, IDC_TOOLBAR_NEW, L"Create a brand new custom folder toolbar. Show off your shortcut collection!");
        return TRUE;
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == LBN_SELCHANGE) SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) { SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR); return TRUE; }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK StartMenuSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HWND hScroll = NULL;
    static ULONG_PTR gdiplusToken = 0;
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        if (!gdiplusToken) {
            GdiplusStartupInput gdiplusStartupInput;
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        }
        
        AddDlgTooltip(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, L"Enable Open-Shell integration fallback when in replace mode. Classic Start Menu experience.");
        AddDlgTooltip(hwndDlg, IDC_MIGRATE_START_MENU_SETTINGS, L"Migrate per-monitor Start Menu settings from the Multi-Monitor tab into this tab.");
        
        hScroll = CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
        
        HKEY hKey;
        DWORD fallbackValue = 1; // Default to 1
        DWORD migrateValue = 1;  // Default to 1
        
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"FallbackStartMenuEnabled", NULL, NULL, (LPBYTE)&fallbackValue, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"MigrateStartMenuSettings", NULL, NULL, (LPBYTE)&migrateValue, &cbData);
            RegCloseKey(hKey);
        }
        
        SendDlgItemMessageW(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, BM_SETCHECK, fallbackValue ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_MIGRATE_START_MENU_SETTINGS, BM_SETCHECK, migrateValue ? BST_CHECKED : BST_UNCHECKED, 0);
        
        if (migrateValue == 1) {
            if (g_Monitors.empty()) EnumDisplayMonitors(NULL, NULL, TaskbarPropsMonitorEnumProc, 0);
            
            int y = 5;
            HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
            
            RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);
            
            for (const auto& mon : g_Monitors) {
                WCHAR title[64];
                wsprintfW(title, L"Monitor %d (%dx%d)", mon.index, mon.rect.right - mon.rect.left, mon.rect.bottom - mon.rect.top);
                HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, 135, hScroll, NULL, g_hInstance, NULL);
                SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
                
                HWND hLblMode = CreateWindowExW(0, L"Static", L"Start Menu Mode:", WS_CHILD | WS_VISIBLE, 15, y + 25, 120, 15, hScroll, NULL, g_hInstance, NULL);
                HWND hLblTrig = CreateWindowExW(0, L"Static", L"Start Menu Trigger:", WS_CHILD | WS_VISIBLE, 15, y + 50, 120, 15, hScroll, NULL, g_hInstance, NULL);
                HWND hLblOrb = CreateWindowExW(0, L"Static", L"Start Orb Theme:", WS_CHILD | WS_VISIBLE, 15, y + 75, 120, 15, hScroll, NULL, g_hInstance, NULL);

                SendMessageW(hLblMode, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hLblTrig, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hLblOrb, WM_SETFONT, (WPARAM)hFont, 0);

                HWND hCmbMode = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 20, 100, 100, hScroll, (HMENU)(ID_BASE_SM_MODE + mon.index), g_hInstance, NULL);
                HWND hCmbTrig = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 45, 100, 100, hScroll, (HMENU)(ID_BASE_SM_TRIG + mon.index), g_hInstance, NULL);
                HWND hCmbOrb = CreateWindowExW(0, L"ComboBox", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 140, y + 70, 100, 100, hScroll, (HMENU)(ID_BASE_SM_ORB + mon.index), g_hInstance, NULL);
                HWND hPreview = CreateWindowExW(0, L"Static", L"", WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | WS_BORDER, 250, y + 45, 54, 54, hScroll, (HMENU)(ID_BASE_SM_PREV + mon.index), g_hInstance, NULL);

                AddTooltip(hScroll, hCmbMode, L"Choose which Start Menu to open on this monitor. Make it your own.");
                AddTooltip(hScroll, hCmbTrig, L"Select mouse/keyboard trigger to summon the Start Menu.");
                AddTooltip(hScroll, hCmbOrb, L"Pick the graphic theme for your Start Orb. Show some style!");
                AddTooltip(hScroll, hPreview, L"A preview of your selected Start Orb theme. Looks sharp!");

                SetWindowSubclass(hCmbMode, NoMouseWheelSubclassProc, 1, 0);
                SetWindowSubclass(hCmbTrig, NoMouseWheelSubclassProc, 1, 0);
                SetWindowSubclass(hCmbOrb, NoMouseWheelSubclassProc, 1, 0);

                SendMessageW(hCmbMode, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hCmbTrig, WM_SETFONT, (WPARAM)hFont, 0);
                SendMessageW(hCmbOrb, WM_SETFONT, (WPARAM)hFont, 0);

                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Native Injection");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Native Windows Start Menu");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Elite Custom Menu");
                SendMessageW(hCmbMode, CB_ADDSTRING, 0, (LPARAM)L"Open-Shell Standalone");

                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Left Click");
                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Middle Click");
                SendMessageW(hCmbTrig, CB_ADDSTRING, 0, (LPARAM)L"Win Key");

                PopulateOrbComboBox(hCmbOrb);
                
                if (hKey) {
                    DWORD mode = 0, cbData = sizeof(DWORD);
                    WCHAR val[64];
                    wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                    if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&mode, &cbData) != ERROR_SUCCESS) {
                        cbData = sizeof(DWORD);
                        RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&mode, &cbData);
                    }
                    SendMessageW(hCmbMode, CB_SETCURSEL, mode, 0);

                    DWORD trig = 0; cbData = sizeof(DWORD);
                    wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                    if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&trig, &cbData) != ERROR_SUCCESS) {
                        cbData = sizeof(DWORD);
                        RegQueryValueExW(hKey, L"StartMenuTrigger", NULL, NULL, (LPBYTE)&trig, &cbData);
                    }
                    SendMessageW(hCmbTrig, CB_SETCURSEL, trig, 0);

                    DWORD orb = 0; cbData = sizeof(DWORD);
                    wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                    if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&orb, &cbData) == ERROR_SUCCESS) {
                        SelectOrbComboBox(hCmbOrb, orb);
                    } else {
                        SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                        orb = IDB_START_ORB;
                    }
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orb);
                    SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                } else {
                    SendMessageW(hCmbOrb, CB_SETCURSEL, 0, 0);
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(IDB_START_ORB);
                    SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                    SendMessageW(hCmbMode, CB_SETCURSEL, 0, 0);
                    SendMessageW(hCmbTrig, CB_SETCURSEL, 0, 0);
                }
                
                y += 145;
            }
            if (hKey) RegCloseKey(hKey);
            
            SCROLLINFO si = { sizeof(si), SIF_RANGE | SIF_PAGE, 0, y, 200 };
            SetScrollInfo(hScroll, SB_VERT, &si, TRUE);
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            
            int id = LOWORD(wParam);
            if (HIWORD(wParam) == CBN_SELCHANGE && id >= ID_BASE_SM_ORB && id < ID_BASE_SM_ORB + 32) {
                int monIndex = id - ID_BASE_SM_ORB;
                HWND hCombo = GetDlgItem(hScroll, id);
                int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    DWORD orbId = SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
                    HWND hPreview = GetDlgItem(hScroll, ID_BASE_SM_PREV + monIndex);
                    HBITMAP hBitmap = LoadPngResourceAsHBITMAP(orbId);
                    HBITMAP hOld = (HBITMAP)SendMessageW(hPreview, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap);
                    if (hOld) DeleteObject(hOld);
                }
            }
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            HKEY hKeyRoot = GetEliteRegistryRoot();
            if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD fallbackVal = (SendDlgItemMessageW(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD migrateVal = (SendDlgItemMessageW(hwndDlg, IDC_MIGRATE_START_MENU_SETTINGS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                
                RegSetValueExW(hKey, L"FallbackStartMenuEnabled", 0, REG_DWORD, (const BYTE*)&fallbackVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"MigrateStartMenuSettings", 0, REG_DWORD, (const BYTE*)&migrateVal, sizeof(DWORD));
                
                if (migrateVal == 1) {
                    for (const auto& mon : g_Monitors) {
                        HWND hMode = GetDlgItem(hScroll, ID_BASE_SM_MODE + mon.index);
                        HWND hTrig = GetDlgItem(hScroll, ID_BASE_SM_TRIG + mon.index);
                        HWND hOrb = GetDlgItem(hScroll, ID_BASE_SM_ORB + mon.index);

                        DWORD mode = hMode ? SendMessageW(hMode, CB_GETCURSEL, 0, 0) : 0;
                        DWORD trig = hTrig ? SendMessageW(hTrig, CB_GETCURSEL, 0, 0) : 0;
                        DWORD orb = hOrb ? GetSelectedOrbID(hOrb) : IDB_START_ORB;

                        WCHAR val[64];
                        wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                        wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                        wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                        RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&orb, sizeof(DWORD));
                    }
                }
                RegCloseKey(hKey);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

#pragma comment(lib, "shlwapi.lib")

#ifndef SPI_GETDESKTOPWALLPAPER
#define SPI_GETDESKTOPWALLPAPER 0x0073
#endif

static std::wstring FindThemePathByName(HWND hwndDlg, const std::wstring& name) {
    std::vector<std::wstring> searchPaths;
    wchar_t systemRoot[MAX_PATH];
    if (GetEnvironmentVariableW(L"SystemRoot", systemRoot, MAX_PATH) > 0) {
        searchPaths.push_back(std::wstring(systemRoot) + L"\\Resources\\Themes");
    } else {
        searchPaths.push_back(L"C:\\Windows\\Resources\\Themes");
    }
    wchar_t localAppData[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", localAppData, MAX_PATH) > 0) {
        searchPaths.push_back(std::wstring(localAppData) + L"\\AppData\\Local\\Microsoft\\Windows\\Themes");
    }
    wchar_t localAppPath[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppPath, MAX_PATH) > 0) {
        searchPaths.push_back(std::wstring(localAppPath) + L"\\Microsoft\\Windows\\Themes");
    }
    
    wchar_t customPath[MAX_PATH] = {0};
    GetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, customPath, MAX_PATH);
    if (wcslen(customPath) > 0) {
        std::wstring customPathStr(customPath);
        if (PathIsDirectoryW(customPathStr.c_str())) {
            searchPaths.push_back(customPathStr);
        } else {
            size_t lastSlash = customPathStr.find_last_of(L'\\');
            if (lastSlash != std::wstring::npos) {
                searchPaths.push_back(customPathStr.substr(0, lastSlash));
            }
        }
    }
    
    for (const auto& path : searchPaths) {
        std::wstring fullPath = path + L"\\" + name + L".theme";
        if (GetFileAttributesW(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return fullPath;
        }
    }
    return L"";
}

static void PopulateThemesComboBox(HWND hwndDlg) {
    HWND hCombo = GetDlgItem(hwndDlg, IDC_DESKTOP_THEME_SELECT);
    SendMessageW(hCombo, CB_RESETCONTENT, 0, 0);
    
    std::vector<std::wstring> searchPaths;
    
    wchar_t systemRoot[MAX_PATH];
    if (GetEnvironmentVariableW(L"SystemRoot", systemRoot, MAX_PATH) > 0) {
        searchPaths.push_back(std::wstring(systemRoot) + L"\\Resources\\Themes");
    } else {
        searchPaths.push_back(L"C:\\Windows\\Resources\\Themes");
    }
    
    wchar_t localAppData[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", localAppData, MAX_PATH) > 0) {
        searchPaths.push_back(std::wstring(localAppData) + L"\\AppData\\Local\\Microsoft\\Windows\\Themes");
    }
    
    wchar_t localAppPath[MAX_PATH];
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", localAppPath, MAX_PATH) > 0) {
        searchPaths.push_back(std::wstring(localAppPath) + L"\\Microsoft\\Windows\\Themes");
    }
    
    wchar_t customPath[MAX_PATH] = {0};
    GetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, customPath, MAX_PATH);
    if (wcslen(customPath) > 0) {
        std::wstring customPathStr(customPath);
        if (PathIsDirectoryW(customPathStr.c_str())) {
            searchPaths.push_back(customPathStr);
        } else {
            size_t lastSlash = customPathStr.find_last_of(L'\\');
            if (lastSlash != std::wstring::npos) {
                searchPaths.push_back(customPathStr.substr(0, lastSlash));
            }
        }
    }
    
    for (const auto& path : searchPaths) {
        if (path.empty() || !PathFileExistsW(path.c_str())) continue;
        std::wstring query = path + L"\\*.theme";
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(query.c_str(), &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                std::wstring themeName = fd.cFileName;
                size_t dot = themeName.find_last_of(L'.');
                if (dot != std::wstring::npos) {
                    themeName = themeName.substr(0, dot);
                }
                if (SendMessageW(hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)themeName.c_str()) == CB_ERR) {
                    SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)themeName.c_str());
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
    }
}

static HICON LoadThemeIcon(const std::wstring& rawPath) {
    if (rawPath.empty()) return NULL;
    
    wchar_t expandedPath[MAX_PATH];
    ExpandEnvironmentStringsW(rawPath.c_str(), expandedPath, MAX_PATH);
    
    std::wstring pathStr(expandedPath);
    int index = 0;
    size_t comma = pathStr.find_last_of(L',');
    if (comma != std::wstring::npos) {
        std::wstring indexStr = pathStr.substr(comma + 1);
        index = _wtoi(indexStr.c_str());
        pathStr = pathStr.substr(0, comma);
    }
    
    HICON hIcon = NULL;
    if (ExtractIconExW(pathStr.c_str(), index, &hIcon, NULL, 1) > 0 && hIcon != NULL) {
        return hIcon;
    }
    
    return (HICON)LoadImageW(NULL, (LPCWSTR)IDI_APPLICATION, IMAGE_ICON, 32, 32, LR_SHARED);
}

static void UpdateIconPreviews(HWND hwndDlg, const std::wstring& themePath) {
    const wchar_t* clsids[] = {
        L"CLSID\\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\DefaultIcon",
        L"CLSID\\{59031A47-3F72-44A7-89C5-5595FE6B30EE}\\DefaultIcon",
        L"CLSID\\{F02C10A9-C50E-10D5-9A7A-00105A2F161D}\\DefaultIcon",
        L"CLSID\\{645FF040-5081-101B-9F08-00AA002F954E}\\DefaultIcon"
    };
    
    int previewIds[] = {
        IDC_DESKTOP_ICON_PREVIEW_1,
        IDC_DESKTOP_ICON_PREVIEW_2,
        IDC_DESKTOP_ICON_PREVIEW_3,
        IDC_DESKTOP_ICON_PREVIEW_4
    };
    
    for (int i = 0; i < 4; i++) {
        wchar_t iconPath[MAX_PATH] = {0};
        GetPrivateProfileStringW(clsids[i], L"DefaultValue", L"", iconPath, MAX_PATH, themePath.c_str());
        HICON hIcon = LoadThemeIcon(iconPath);
        if (hIcon) {
            HICON hOldIcon = (HICON)SendDlgItemMessageW(hwndDlg, previewIds[i], STM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);
            if (hOldIcon) DestroyIcon(hOldIcon);
        }
    }
}

static std::wstring GetWallpaperPathFromTheme(const std::wstring& themePath) {
    wchar_t wallpaperPath[MAX_PATH] = {0};
    GetPrivateProfileStringW(L"Control Panel\\Desktop", L"Wallpaper", L"", wallpaperPath, MAX_PATH, themePath.c_str());
    if (wcslen(wallpaperPath) > 0) {
        wchar_t expanded[MAX_PATH];
        ExpandEnvironmentStringsW(wallpaperPath, expanded, MAX_PATH);
        return expanded;
    }
    return L"";
}

static std::wstring GetActiveWallpaperPath(HWND hwndDlg) {
    HWND hCombo = GetDlgItem(hwndDlg, IDC_DESKTOP_THEME_SELECT);
    int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
    if (sel != CB_ERR) {
        wchar_t themeName[256];
        SendMessageW(hCombo, CB_GETLBTEXT, sel, (LPARAM)themeName);
        std::wstring themePath = FindThemePathByName(hwndDlg, themeName);
        if (!themePath.empty()) {
            std::wstring wall = GetWallpaperPathFromTheme(themePath);
            if (!wall.empty() && PathFileExistsW(wall.c_str())) {
                return wall;
            }
        }
    }
    wchar_t sysWallpaper[MAX_PATH] = {0};
    if (SystemParametersInfoW(SPI_GETDESKTOPWALLPAPER, MAX_PATH, sysWallpaper, 0)) {
        return sysWallpaper;
    }
    return L"";
}

static void DrawWallpaperPreview(HWND hwndDlg, HDC hdc, const RECT* prc) {
    HBRUSH hBg = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
    FillRect(hdc, prc, hBg);
    DeleteObject(hBg);
    
    std::wstring path = GetActiveWallpaperPath(hwndDlg);
    if (!path.empty() && PathFileExistsW(path.c_str())) {
        Gdiplus::Graphics graphics(hdc);
        Gdiplus::Bitmap bitmap(path.c_str());
        if (bitmap.GetLastStatus() == Gdiplus::Ok) {
            int imgW = bitmap.GetWidth();
            int imgH = bitmap.GetHeight();
            
            int prcW = prc->right - prc->left;
            int prcH = prc->bottom - prc->top;
            
            double imgAspect = (double)imgW / imgH;
            double prcAspect = (double)prcW / prcH;
            
            int destW, destH;
            if (imgAspect > prcAspect) {
                destW = prcW;
                destH = (int)(prcW / imgAspect);
            } else {
                destH = prcH;
                destW = (int)(prcH * imgAspect);
            }
            
            int destX = prc->left + (prcW - destW) / 2;
            int destY = prc->top + (prcH - destH) / 2;
            
            graphics.DrawImage(&bitmap, destX, destY, destW, destH);
        }
    } else {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
        HFONT hFont = (HFONT)SendMessageW(hwndDlg, WM_GETFONT, 0, 0);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        DrawTextW(hdc, L"No Wallpaper Preview Available", -1, (LPRECT)prc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, hOldFont);
    }
}

static void LoadThemeSettings(HWND hwndDlg, const std::wstring& themePath) {
    if (themePath.empty() || !PathFileExistsW(themePath.c_str())) return;
    SetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, themePath.c_str());
    
    HWND hCombo = GetDlgItem(hwndDlg, IDC_DESKTOP_THEME_SELECT);
    size_t dot = themePath.find_last_of(L'.');
    size_t slash = themePath.find_last_of(L'\\');
    if (slash != std::wstring::npos && dot != std::wstring::npos && dot > slash) {
        std::wstring name = themePath.substr(slash + 1, dot - slash - 1);
        int idx = (int)SendMessageW(hCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)name.c_str());
        if (idx != CB_ERR) {
            SendMessageW(hCombo, CB_SETCURSEL, idx, 0);
        }
    }
    UpdateIconPreviews(hwndDlg, themePath);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_DESKTOP_WALLPAPER_PREVIEW), NULL, TRUE);
}

static std::wstring BrowseForFolder(HWND hwndOwner) {
    wchar_t szDir[MAX_PATH] = {0};
    BROWSEINFOW bi = {0};
    bi.hwndOwner = hwndOwner;
    bi.pszDisplayName = szDir;
    bi.lpszTitle = L"Select Theme Directory:";
    bi.ulFlags = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
    
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl != NULL) {
        SHGetPathFromIDListW((PCIDLIST_ABSOLUTE)pidl, szDir);
        CoTaskMemFree(pidl);
        return szDir;
    }
    return L"";
}

INT_PTR CALLBACK DesktopSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static ULONG_PTR gdiplusToken = 0;
    switch (uMsg) {
    case WM_DESTROY: {
        int previewIds[] = {
            IDC_DESKTOP_ICON_PREVIEW_1,
            IDC_DESKTOP_ICON_PREVIEW_2,
            IDC_DESKTOP_ICON_PREVIEW_3,
            IDC_DESKTOP_ICON_PREVIEW_4
        };
        for (int i = 0; i < 4; i++) {
            HICON hIcon = (HICON)SendDlgItemMessageW(hwndDlg, previewIds[i], STM_GETIMAGE, IMAGE_ICON, 0);
            if (hIcon) {
                DestroyIcon(hIcon);
            }
        }
        if (gdiplusToken) {
            Gdiplus::GdiplusShutdown(gdiplusToken);
            gdiplusToken = 0;
        }
        break;
    }
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        if (!gdiplusToken) {
            Gdiplus::GdiplusStartupInput gdiplusStartupInput;
            Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
        }
        
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, L"Enable custom desktop replacement window. Pure classic shell experience.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, L"Render desktop background wallpaper using custom styles.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, L"Display folder items and files on the desktop grid.");
        
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_FORCE_PROGMAN_ALL, L"Forcibly injects our Progman replacement onto every screen. Because one display is never enough for Elite performance.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_MODE_SPAN, L"Stretches your wallpaper across your entire multi-monitor setup, hoping the aspect ratios align.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, L"Allows each display to render its own distinct wallpaper, like a true professional workspace.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_PATH, L"Specifies the directory path of your custom desktop wallpaper themes. Choose wisely.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_BROWSE, L"Browse directory folders to discover where your hidden themes are stored.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_SELECT, L"Select your active visual theme. Instant aesthetic upgrade.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_SLIDESHOW_ENABLED, L"Periodically rotates wallpapers. Keeps your eyes occupied while the system works.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_SLIDESHOW_INTERVAL, L"Select how long a single wallpaper remains before being unceremoniously swapped out.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_THEME_TUTORIAL_LINK, L"Learn how to craft themes. Warning: may lead to retro design obsessions.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_WALLPAPER_PREVIEW, L"Your tiny, non-interactive window to the aesthetic soul of your machine.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_ICON_PREVIEW_1, L"Your computer icon representation. Reminding you that you own this hardware.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_ICON_PREVIEW_2, L"Your user files icon representation. A digital box for your files.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_ICON_PREVIEW_3, L"Your network icon representation. Connecting you to the web.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_ICON_PREVIEW_4, L"Your recycle bin icon representation. Where bad code goes to die.");
        AddDlgTooltip(hwndDlg, IDC_USE_NATIVE_WALLPAPER, L"Switch between the native Windows wallpaper engine (default) and our retro custom engine.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_THUMBNAILS, L"Render dynamic file previews/thumbnails on the desktop. Premium vibes included.");
        
        HKEY hKey;
        DWORD replaceVal = 1;
        DWORD wallpaperVal = 1;
        DWORD iconsVal = 1;
        DWORD forceProgmanVal = 0;
        DWORD wallpaperModeVal = 0;
        wchar_t themePathVal[MAX_PATH] = {0};
        DWORD slideshowEnabledVal = 0;
        DWORD slideshowIntervalVal = 300;
        DWORD useNativeWallpaperVal = 1;
        DWORD thumbnailsVal = 1;
        DWORD cbData = sizeof(DWORD);
        
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopThumbnailsEnabled", NULL, NULL, (LPBYTE)&thumbnailsVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopReplacementEnabled", NULL, NULL, (LPBYTE)&replaceVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopWallpaperEnabled", NULL, NULL, (LPBYTE)&wallpaperVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopIconsEnabled", NULL, NULL, (LPBYTE)&iconsVal, &cbData);
            
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"ForceProgmanAllDisplays", NULL, NULL, (LPBYTE)&forceProgmanVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopWallpaperMode", NULL, NULL, (LPBYTE)&wallpaperModeVal, &cbData);
            
            cbData = MAX_PATH * sizeof(wchar_t);
            RegQueryValueExW(hKey, L"DesktopThemePath", NULL, NULL, (LPBYTE)themePathVal, &cbData);
            
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopSlideshowEnabled", NULL, NULL, (LPBYTE)&slideshowEnabledVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopSlideshowInterval", NULL, NULL, (LPBYTE)&slideshowIntervalVal, &cbData);
            
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"UseNativeWallpaperEngine", NULL, NULL, (LPBYTE)&useNativeWallpaperVal, &cbData) != ERROR_SUCCESS) {
                useNativeWallpaperVal = 1;
            }
            
            RegCloseKey(hKey);
        }
        
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, BM_SETCHECK, replaceVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_SETCHECK, wallpaperVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, BM_SETCHECK, iconsVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_THUMBNAILS, BM_SETCHECK, thumbnailsVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_USE_NATIVE_WALLPAPER, BM_SETCHECK, useNativeWallpaperVal ? BST_CHECKED : BST_UNCHECKED, 0);
        
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_FORCE_PROGMAN_ALL, BM_SETCHECK, forceProgmanVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_SPAN, BM_SETCHECK, (wallpaperModeVal == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, BM_SETCHECK, (wallpaperModeVal == 1) ? BST_CHECKED : BST_UNCHECKED, 0);
        
        SetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, themePathVal);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_SLIDESHOW_ENABLED, BM_SETCHECK, slideshowEnabledVal ? BST_CHECKED : BST_UNCHECKED, 0);
        
        HWND hIntervalCombo = GetDlgItem(hwndDlg, IDC_DESKTOP_SLIDESHOW_INTERVAL);
        const wchar_t* intervals[] = { L"10", L"30", L"60", L"300", L"600", L"1800", L"3600" };
        for (const auto& val : intervals) {
            SendMessageW(hIntervalCombo, CB_ADDSTRING, 0, (LPARAM)val);
        }
        wchar_t intervalBuf[32];
        swprintf_s(intervalBuf, L"%lu", slideshowIntervalVal);
        SetWindowTextW(hIntervalCombo, intervalBuf);
        
        PopulateThemesComboBox(hwndDlg);
        if (wcslen(themePathVal) > 0) {
            LoadThemeSettings(hwndDlg, themePathVal);
        }
        
        return TRUE;
    }
    case WM_COMMAND: {
        WORD wNotifyCode = HIWORD(wParam);
        WORD wID = LOWORD(wParam);
        
        if (wNotifyCode == BN_CLICKED || wNotifyCode == CBN_SELCHANGE || wNotifyCode == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        
        if (wNotifyCode == BN_CLICKED) {
            if (wID == IDC_DESKTOP_THEME_BROWSE) {
                std::wstring folder = BrowseForFolder(hwndDlg);
                if (!folder.empty()) {
                    SetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, folder.c_str());
                    PopulateThemesComboBox(hwndDlg);
                    SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
                }
            }
        }
        
        if (wNotifyCode == CBN_SELCHANGE) {
            if (wID == IDC_DESKTOP_THEME_SELECT) {
                HWND hCombo = (HWND)lParam;
                int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    wchar_t themeName[256];
                    SendMessageW(hCombo, CB_GETLBTEXT, sel, (LPARAM)themeName);
                    std::wstring themePath = FindThemePathByName(hwndDlg, themeName);
                    if (!themePath.empty()) {
                        SetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, themePath.c_str());
                        UpdateIconPreviews(hwndDlg, themePath);
                        
                        wchar_t wallpaperPath[MAX_PATH] = {0};
                        GetPrivateProfileStringW(L"Control Panel\\Desktop", L"Wallpaper", L"", wallpaperPath, MAX_PATH, themePath.c_str());
                        if (wcslen(wallpaperPath) > 0) {
                            SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_SETCHECK, BST_CHECKED, 0);
                        }
                        
                        wchar_t styleVal[32] = {0};
                        GetPrivateProfileStringW(L"Control Panel\\Desktop", L"WallpaperStyle", L"", styleVal, 32, themePath.c_str());
                        int styleNum = _wtoi(styleVal);
                        if (styleNum == 22) {
                            SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_SPAN, BM_SETCHECK, BST_CHECKED, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, BM_SETCHECK, BST_UNCHECKED, 0);
                        } else {
                            SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_SPAN, BM_SETCHECK, BST_UNCHECKED, 0);
                            SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, BM_SETCHECK, BST_CHECKED, 0);
                        }
                        InvalidateRect(GetDlgItem(hwndDlg, IDC_DESKTOP_WALLPAPER_PREVIEW), NULL, TRUE);
                    }
                }
            }
        }
        return TRUE;
    }
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT* pDIS = (DRAWITEMSTRUCT*)lParam;
        if (pDIS->CtlID == IDC_DESKTOP_WALLPAPER_PREVIEW) {
            DrawWallpaperPreview(hwndDlg, pDIS->hDC, &pDIS->rcItem);
            return TRUE;
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            HKEY hKeyRoot = GetEliteRegistryRoot();
            if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD replaceVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD wallpaperVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD iconsVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                
                DWORD forceProgmanVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_FORCE_PROGMAN_ALL, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD wallpaperModeVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_PERMONITOR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                
                wchar_t themePathVal[MAX_PATH] = {0};
                GetDlgItemTextW(hwndDlg, IDC_DESKTOP_THEME_PATH, themePathVal, MAX_PATH);
                
                DWORD slideshowEnabledVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_SLIDESHOW_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                
                wchar_t intervalBuf[32] = {0};
                GetDlgItemTextW(hwndDlg, IDC_DESKTOP_SLIDESHOW_INTERVAL, intervalBuf, 32);
                DWORD slideshowIntervalVal = _wtoi(intervalBuf);
                if (slideshowIntervalVal < 3) slideshowIntervalVal = 3;
                
                DWORD useNativeWallpaperVal = (SendDlgItemMessageW(hwndDlg, IDC_USE_NATIVE_WALLPAPER, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD thumbnailsVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_THUMBNAILS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;

                RegSetValueExW(hKey, L"DesktopReplacementEnabled", 0, REG_DWORD, (const BYTE*)&replaceVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopWallpaperEnabled", 0, REG_DWORD, (const BYTE*)&wallpaperVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopIconsEnabled", 0, REG_DWORD, (const BYTE*)&iconsVal, sizeof(DWORD));
                
                RegSetValueExW(hKey, L"ForceProgmanAllDisplays", 0, REG_DWORD, (const BYTE*)&forceProgmanVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopWallpaperMode", 0, REG_DWORD, (const BYTE*)&wallpaperModeVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopThemePath", 0, REG_SZ, (const BYTE*)themePathVal, (DWORD)(wcslen(themePathVal) + 1) * sizeof(wchar_t));
                RegSetValueExW(hKey, L"DesktopSlideshowEnabled", 0, REG_DWORD, (const BYTE*)&slideshowEnabledVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopSlideshowInterval", 0, REG_DWORD, (const BYTE*)&slideshowIntervalVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"UseNativeWallpaperEngine", 0, REG_DWORD, (const BYTE*)&useNativeWallpaperVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopThumbnailsEnabled", 0, REG_DWORD, (const BYTE*)&thumbnailsVal, sizeof(DWORD));
                
                RegCloseKey(hKey);
            }
            
            {
                std::wstring wallpaperPath = L"";
                if (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    wallpaperPath = GetActiveWallpaperPath(hwndDlg);
                }
                
                std::wstring styleStr = L"22";
                std::wstring tileStr = L"0";
                
                HKEY hKeyDesktop;
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKeyDesktop) == ERROR_SUCCESS) {
                    wchar_t szBuffer[MAX_PATH] = { 0 };
                    DWORD dwSize = sizeof(szBuffer);
                    if (RegQueryValueExW(hKeyDesktop, L"WallpaperStyle", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                        styleStr = szBuffer;
                    }
                    dwSize = sizeof(szBuffer);
                    if (RegQueryValueExW(hKeyDesktop, L"TileWallpaper", NULL, NULL, (LPBYTE)szBuffer, &dwSize) == ERROR_SUCCESS) {
                        tileStr = szBuffer;
                    }
                    RegCloseKey(hKeyDesktop);
                }
                
                HWND hCombo = GetDlgItem(hwndDlg, IDC_DESKTOP_THEME_SELECT);
                int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    wchar_t themeName[256];
                    SendMessageW(hCombo, CB_GETLBTEXT, sel, (LPARAM)themeName);
                    std::wstring themePath = FindThemePathByName(hwndDlg, themeName);
                    if (!themePath.empty()) {
                        wchar_t wstyle[32] = {0};
                        wchar_t wtile[32] = {0};
                        GetPrivateProfileStringW(L"Control Panel\\Desktop", L"WallpaperStyle", L"", wstyle, 32, themePath.c_str());
                        GetPrivateProfileStringW(L"Control Panel\\Desktop", L"TileWallpaper", L"", wtile, 32, themePath.c_str());
                        if (wcslen(wstyle) > 0) styleStr = wstyle;
                        if (wcslen(wtile) > 0) tileStr = wtile;
                    }
                }
                
                if (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_MODE_SPAN, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    styleStr = L"22";
                    tileStr = L"0";
                } else {
                    if (styleStr == L"22") {
                        styleStr = L"10";
                    }
                }
                
                HKEY hKeyDesktopWrite;
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &hKeyDesktopWrite) == ERROR_SUCCESS) {
                    RegSetValueExW(hKeyDesktopWrite, L"Wallpaper", 0, REG_SZ, (const BYTE*)wallpaperPath.c_str(), (DWORD)(wallpaperPath.length() + 1) * sizeof(wchar_t));
                    RegSetValueExW(hKeyDesktopWrite, L"WallpaperStyle", 0, REG_SZ, (const BYTE*)styleStr.c_str(), (DWORD)(styleStr.length() + 1) * sizeof(wchar_t));
                    RegSetValueExW(hKeyDesktopWrite, L"TileWallpaper", 0, REG_SZ, (const BYTE*)tileStr.c_str(), (DWORD)(tileStr.length() + 1) * sizeof(wchar_t));
                    RegCloseKey(hKeyDesktopWrite);
                }
                
                SystemParametersInfoW(SPI_SETDESKWALLPAPER, 0, (void*)wallpaperPath.c_str(), SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
            }
            NotifySettingsChange();
            SendMessageW(GetParent(hwndDlg), PSM_UNCHANGED, (WPARAM)hwndDlg, 0);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        else if (lpnm->code == NM_CLICK || lpnm->code == NM_RETURN) {
            if (lpnm->idFrom == IDC_DESKTOP_THEME_TUTORIAL_LINK) {
                ShellExecuteW(NULL, L"open", L"https://elitesoftwaretech.com/themes-tutorial", NULL, NULL, SW_SHOWNORMAL);
                return TRUE;
            }
        }
        break;
    }
    }
    return FALSE;
}

void ExportSettingsToXML(HWND hwndOwner) {
    OPENFILENAMEW ofn = { 0 };
    wchar_t szFile[MAX_PATH] = L"EliteTaskbarSettings.xml";
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"XML Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn)) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, szFile, L"w, ccs=UTF-8") == 0 && f) {
            fwprintf(f, L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
            fwprintf(f, L"<EliteTaskbarSettings>\n");

            HKEY hKey;
            if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                WCHAR valName[MAX_PATH];
                DWORD cbValName = MAX_PATH;
                DWORD dwType = 0;
                BYTE dwData[1024];
                DWORD cbData = sizeof(dwData);
                DWORD dwIndex = 0;
                while (RegEnumValueW(hKey, dwIndex, valName, &cbValName, NULL, &dwType, dwData, &cbData) == ERROR_SUCCESS) {
                    if (dwType == REG_DWORD) {
                        DWORD dwVal = *(DWORD*)dwData;
                        fwprintf(f, L"    <Setting name=\"%s\" type=\"DWORD\">%u</Setting>\n", valName, dwVal);
                    }
                    else if (dwType == REG_SZ) {
                        wchar_t* szVal = (wchar_t*)dwData;
                        fwprintf(f, L"    <Setting name=\"%s\" type=\"SZ\">%s</Setting>\n", valName, szVal);
                    }
                    dwIndex++;
                    cbValName = MAX_PATH;
                    cbData = sizeof(dwData);
                }
                RegCloseKey(hKey);
            }
            fwprintf(f, L"</EliteTaskbarSettings>\n");
            fclose(f);
            MessageBoxW(hwndOwner, L"Configuration successfully exported to XML.", L"Backup Successful", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(hwndOwner, L"Failed to create the destination XML file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void ImportSettingsFromXML(HWND hwndOwner) {
    OPENFILENAMEW ofn = { 0 };
    wchar_t szFile[MAX_PATH] = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"XML Files (*.xml)\0*.xml\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        FILE* f = nullptr;
        if (_wfopen_s(&f, szFile, L"r, ccs=UTF-8") == 0 && f) {
            HKEY hKey;
            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                wchar_t line[2048];
                while (fgetws(line, 2048, f)) {
                    std::wstring str(line);
                    size_t posName = str.find(L"name=\"");
                    if (posName != std::wstring::npos) {
                        size_t posNameEnd = str.find(L"\"", posName + 6);
                        std::wstring name = str.substr(posName + 6, posNameEnd - (posName + 6));

                        size_t posType = str.find(L"type=\"");
                        size_t posTypeEnd = str.find(L"\"", posType + 6);
                        std::wstring type = str.substr(posType + 6, posTypeEnd - (posType + 6));

                        size_t posValueStart = str.find(L">", posTypeEnd);
                        size_t posValueEnd = str.find(L"</Setting>", posValueStart);
                        std::wstring value = str.substr(posValueStart + 1, posValueEnd - (posValueStart + 1));

                        if (type == L"DWORD") {
                            DWORD dwVal = (DWORD)_wtoi(value.c_str());
                            RegSetValueExW(hKey, name.c_str(), 0, REG_DWORD, (const BYTE*)&dwVal, sizeof(DWORD));
                        }
                        else if (type == L"SZ") {
                            RegSetValueExW(hKey, name.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), (DWORD)(value.length() + 1) * sizeof(wchar_t));
                        }
                    }
                }
                RegCloseKey(hKey);
                MessageBoxW(hwndOwner, L"Configuration successfully imported from XML.", L"Restore Successful", MB_OK | MB_ICONINFORMATION);
            } else {
                MessageBoxW(hwndOwner, L"Failed to open registry for writing.", L"Error", MB_OK | MB_ICONERROR);
            }
            fclose(f);
        } else {
            MessageBoxW(hwndOwner, L"Failed to open XML file for reading.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

INT_PTR CALLBACK ExplorerSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_HIDDEN_FILES, L"Toggle visibility of files hidden by developers or system internals. Use with caution.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_EXTENSIONS, L"Toggle visibility of file extensions. Keep unchecked if you actually want to know what kind of file you're clicking.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_ENABLE_TASKBAR, L"Enable or disable the custom Elite Taskbar shell extension.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_ENABLE_STARTMENU, L"Enable or disable the custom Elite Start Menu launcher integration.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_DEFAULT_GROUPBYTYPE, L"Enable grouping of folder items by file type as the default view arrangement.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_NATIVE_VIEWMODE, L"Toggle between custom high-density listview and Windows native folder views.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_SHELLBAGS, L"Enable tracking of individual folder visual preferences using Shell Bags registry storage.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_NATIVE_SHELLVIEW, L"Toggle hosting the native IShellView desktop or falling back to the legacy SysListView32 grid.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_BACKUP, L"Export current taskbar configuration to XML. Prepare for digital time travel.");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_RESTORE, L"Import taskbar configuration from XML. Restoring previous timeline configurations.");

        HKEY hKey;
        DWORD hiddenVal = 2; // Default to hide (2)
        DWORD extVal = 1;    // Default to hide extensions (1)
        DWORD cbData = sizeof(DWORD);
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"Hidden", NULL, NULL, (LPBYTE)&hiddenVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"HideFileExt", NULL, NULL, (LPBYTE)&extVal, &cbData);
            RegCloseKey(hKey);
        }

        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_HIDDEN_FILES, BM_SETCHECK, (hiddenVal == 1) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_EXTENSIONS, BM_SETCHECK, (extVal == 1) ? BST_CHECKED : BST_UNCHECKED, 0);

        // Load Elite Integration settings - Builder-Bob
        DWORD enableEliteTaskbar = 1;
        DWORD enableEliteStartMenu = 1;
        DWORD enableDefaultGroupByType = 1;
        DWORD enableNativeViewMode = 1;
        DWORD enableShellBagsSupport = 0;
        DWORD useNativeShellView = 1;

        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"EnableEliteTaskbar", NULL, NULL, (LPBYTE)&enableEliteTaskbar, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"EnableEliteStartMenu", NULL, NULL, (LPBYTE)&enableEliteStartMenu, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"EnableDefaultGroupByType", NULL, NULL, (LPBYTE)&enableDefaultGroupByType, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"EnableNativeViewMode", NULL, NULL, (LPBYTE)&enableNativeViewMode, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"EnableShellBagsSupport", NULL, NULL, (LPBYTE)&enableShellBagsSupport, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"UseNativeShellView", NULL, NULL, (LPBYTE)&useNativeShellView, &cbData);
            RegCloseKey(hKey);
        }

        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_ENABLE_TASKBAR, BM_SETCHECK, enableEliteTaskbar ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_ENABLE_STARTMENU, BM_SETCHECK, enableEliteStartMenu ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_DEFAULT_GROUPBYTYPE, BM_SETCHECK, enableDefaultGroupByType ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_NATIVE_VIEWMODE, BM_SETCHECK, enableNativeViewMode ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_SHELLBAGS, BM_SETCHECK, enableShellBagsSupport ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_NATIVE_SHELLVIEW, BM_SETCHECK, useNativeShellView ? BST_CHECKED : BST_UNCHECKED, 0);

        HWND hwndCombo = GetDlgItem(hwndDlg, IDC_EXPLORER_FOLDER_VIEW_DEFAULT);
        SendMessageW(hwndCombo, CB_RESETCONTENT, 0, 0);
        int idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Small Icon Tiles (Default)");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)12);
        idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Tiles");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)5);
        idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Details");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)4);
        idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"List");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)3);
        idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Small Icons");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)2);
        idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Icons");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)1);
        idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Large Icons");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)8);
        idx = (int)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)L"Extra Large Icons");
        SendMessageW(hwndCombo, CB_SETITEMDATA, idx, (LPARAM)7);

        DWORD defaultFolderView = 12;
        DWORD defaultToThumbnailMirrors = 1;
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DefaultFolderView", NULL, NULL, (LPBYTE)&defaultFolderView, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DefaultToThumbnailMirrors", NULL, NULL, (LPBYTE)&defaultToThumbnailMirrors, &cbData);
            RegCloseKey(hKey);
        }

        int count = (int)SendMessageW(hwndCombo, CB_GETCOUNT, 0, 0);
        SendMessageW(hwndCombo, CB_SETCURSEL, 0, 0);
        for (int i = 0; i < count; ++i) {
            if ((DWORD)SendMessageW(hwndCombo, CB_GETITEMDATA, i, 0) == defaultFolderView) {
                SendMessageW(hwndCombo, CB_SETCURSEL, i, 0);
                break;
            }
        }

        SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_THUMBNAIL_MIRROR_VIEWS, BM_SETCHECK, defaultToThumbnailMirrors ? BST_CHECKED : BST_UNCHECKED, 0);
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_FOLDER_VIEW_DEFAULT, L"Select the default folder layout view style (e.g. tiles, details, list).");
        AddDlgTooltip(hwndDlg, IDC_EXPLORER_THUMBNAIL_MIRROR_VIEWS, L"Toggle whether default folder layouts automatically render image/video thumbnails where possible.");

        return TRUE;
    }
    case WM_COMMAND: {
        WORD wNotifyCode = HIWORD(wParam);
        WORD wID = LOWORD(wParam);
        if (wNotifyCode == BN_CLICKED) {
            if (wID == IDC_EXPLORER_HIDDEN_FILES || wID == IDC_EXPLORER_EXTENSIONS ||
                wID == IDC_EXPLORER_ENABLE_TASKBAR || wID == IDC_EXPLORER_ENABLE_STARTMENU ||
                wID == IDC_EXPLORER_DEFAULT_GROUPBYTYPE || wID == IDC_EXPLORER_NATIVE_VIEWMODE ||
                wID == IDC_EXPLORER_SHELLBAGS || wID == IDC_EXPLORER_NATIVE_SHELLVIEW ||
                wID == IDC_EXPLORER_THUMBNAIL_MIRROR_VIEWS || wID == IDC_EXPLORER_FOLDER_VIEW_DEFAULT) {
                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            }
            else if (wID == IDC_EXPLORER_BACKUP) {
                ExportSettingsToXML(hwndDlg);
            }
            else if (wID == IDC_EXPLORER_RESTORE) {
                ImportSettingsFromXML(hwndDlg);
                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
            }
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                DWORD hiddenVal = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_HIDDEN_FILES, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 2;
                DWORD extVal = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_EXTENSIONS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"Hidden", 0, REG_DWORD, (const BYTE*)&hiddenVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"HideFileExt", 0, REG_DWORD, (const BYTE*)&extVal, sizeof(DWORD));
                RegCloseKey(hKey);

                SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSHNOWAIT, NULL, NULL);
            }

            // Save Elite Integration settings - Builder-Bob
            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD enableEliteTaskbar = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_ENABLE_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD enableEliteStartMenu = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_ENABLE_STARTMENU, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD enableDefaultGroupByType = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_DEFAULT_GROUPBYTYPE, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD enableNativeViewMode = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_NATIVE_VIEWMODE, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD enableShellBagsSupport = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_SHELLBAGS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD useNativeShellView = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_NATIVE_SHELLVIEW, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;

                RegSetValueExW(hKey, L"EnableEliteTaskbar", 0, REG_DWORD, (const BYTE*)&enableEliteTaskbar, sizeof(DWORD));
                RegSetValueExW(hKey, L"EnableEliteStartMenu", 0, REG_DWORD, (const BYTE*)&enableEliteStartMenu, sizeof(DWORD));
                RegSetValueExW(hKey, L"EnableDefaultGroupByType", 0, REG_DWORD, (const BYTE*)&enableDefaultGroupByType, sizeof(DWORD));
                RegSetValueExW(hKey, L"EnableNativeViewMode", 0, REG_DWORD, (const BYTE*)&enableNativeViewMode, sizeof(DWORD));
                RegSetValueExW(hKey, L"EnableShellBagsSupport", 0, REG_DWORD, (const BYTE*)&enableShellBagsSupport, sizeof(DWORD));
                RegSetValueExW(hKey, L"UseNativeShellView", 0, REG_DWORD, (const BYTE*)&useNativeShellView, sizeof(DWORD));

                HWND hwndCombo = GetDlgItem(hwndDlg, IDC_EXPLORER_FOLDER_VIEW_DEFAULT);
                int sel = (int)SendMessageW(hwndCombo, CB_GETCURSEL, 0, 0);
                DWORD defaultFolderView = 12;
                if (sel != CB_ERR) {
                    defaultFolderView = (DWORD)SendMessageW(hwndCombo, CB_GETITEMDATA, sel, 0);
                }
                RegSetValueExW(hKey, L"DefaultFolderView", 0, REG_DWORD, (const BYTE*)&defaultFolderView, sizeof(DWORD));

                DWORD defaultToThumbnailMirrors = (SendDlgItemMessageW(hwndDlg, IDC_EXPLORER_THUMBNAIL_MIRROR_VIEWS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"DefaultToThumbnailMirrors", 0, REG_DWORD, (const BYTE*)&defaultToThumbnailMirrors, sizeof(DWORD));

                RegCloseKey(hKey);
            }

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK DWMSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        AddDlgTooltip(hwndDlg, IDC_DWM_ANIMATIONS, L"Toggle slick window animations. Turn off if your graphics card is from the stone age.");
        AddDlgTooltip(hwndDlg, IDC_DWM_GLASS, L"Enable beautiful transparent Aero Glass borders. Flat design fans look away.");
        AddDlgTooltip(hwndDlg, IDC_DWM_BORDER_SIZE, L"Choose window border thickness. Classic Aero values range from 4px to 12px.");

        HWND hCombo = GetDlgItem(hwndDlg, IDC_DWM_BORDER_SIZE);
        int idx1 = (int)SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Classic (1px)");
        SendMessageW(hCombo, CB_SETITEMDATA, idx1, 1);
        int idx2 = (int)SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Standard (4px)");
        SendMessageW(hCombo, CB_SETITEMDATA, idx2, 4);
        int idx3 = (int)SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Aero Wide (8px)");
        SendMessageW(hCombo, CB_SETITEMDATA, idx3, 8);
        int idx4 = (int)SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Legacy Thick (12px)");
        SendMessageW(hCombo, CB_SETITEMDATA, idx4, 12);

        HKEY hKey;
        DWORD animationsVal = 1;
        DWORD glassVal = 1;
        DWORD borderSizeVal = 4;
        DWORD cbData = sizeof(DWORD);
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DwmAnimationsEnabled", NULL, NULL, (LPBYTE)&animationsVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DwmGlassEnabled", NULL, NULL, (LPBYTE)&glassVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DwmBorderSize", NULL, NULL, (LPBYTE)&borderSizeVal, &cbData);
            RegCloseKey(hKey);
        }

        SendDlgItemMessageW(hwndDlg, IDC_DWM_ANIMATIONS, BM_SETCHECK, animationsVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DWM_GLASS, BM_SETCHECK, glassVal ? BST_CHECKED : BST_UNCHECKED, 0);

        int count = (int)SendMessageW(hCombo, CB_GETCOUNT, 0, 0);
        for (int i = 0; i < count; i++) {
            if ((DWORD)SendMessageW(hCombo, CB_GETITEMDATA, i, 0) == borderSizeVal) {
                SendMessageW(hCombo, CB_SETCURSEL, i, 0);
                break;
            }
        }
        return TRUE;
    }
    case WM_COMMAND: {
        WORD wNotifyCode = HIWORD(wParam);
        if (wNotifyCode == BN_CLICKED || wNotifyCode == CBN_SELCHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD animationsVal = (SendDlgItemMessageW(hwndDlg, IDC_DWM_ANIMATIONS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD glassVal = (SendDlgItemMessageW(hwndDlg, IDC_DWM_GLASS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                
                HWND hCombo = GetDlgItem(hwndDlg, IDC_DWM_BORDER_SIZE);
                int sel = (int)SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
                DWORD borderSizeVal = 4;
                if (sel != CB_ERR) {
                    borderSizeVal = (DWORD)SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
                }

                RegSetValueExW(hKey, L"DwmAnimationsEnabled", 0, REG_DWORD, (const BYTE*)&animationsVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DwmGlassEnabled", 0, REG_DWORD, (const BYTE*)&glassVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DwmBorderSize", 0, REG_DWORD, (const BYTE*)&borderSizeVal, sizeof(DWORD));
                RegCloseKey(hKey);
            }
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK ColorsSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        AddDlgTooltip(hwndDlg, IDC_COLORS_HIGHLIGHT, L"Specify your highlight tint in Hex color format. Standard format for maximum customization.");
        AddDlgTooltip(hwndDlg, IDC_COLORS_CLASSIC, L"Force Windows Aero highlight styling and classic gradients on taskbar elements.");
        AddDlgTooltip(hwndDlg, IDC_COLORS_WINDOWS_DARK, L"Toggle dark theme style for Windows shell components and Taskbar.");
        AddDlgTooltip(hwndDlg, IDC_COLORS_APP_DARK, L"Toggle dark theme style for supporting applications.");

        HKEY hKey;
        wchar_t highlightVal[32] = L"0078D7";
        DWORD classicColorsVal = 0;
        DWORD cbData = sizeof(classicColorsVal);
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(classicColorsVal);
            RegQueryValueExW(hKey, L"ClassicColorsEnabled", NULL, NULL, (LPBYTE)&classicColorsVal, &cbData);
            cbData = sizeof(highlightVal);
            RegQueryValueExW(hKey, L"HighlightTint", NULL, NULL, (LPBYTE)highlightVal, &cbData);
            RegCloseKey(hKey);
        }

        SetDlgItemTextW(hwndDlg, IDC_COLORS_HIGHLIGHT, highlightVal);
        SendDlgItemMessageW(hwndDlg, IDC_COLORS_CLASSIC, BM_SETCHECK, classicColorsVal ? BST_CHECKED : BST_UNCHECKED, 0);

        DWORD sysUsesLightTheme = 0;
        DWORD appsUseLightTheme = 1;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"SystemUsesLightTheme", NULL, NULL, (LPBYTE)&sysUsesLightTheme, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&appsUseLightTheme, &cbData);
            RegCloseKey(hKey);
        }

        SendDlgItemMessageW(hwndDlg, IDC_COLORS_WINDOWS_DARK, BM_SETCHECK, (sysUsesLightTheme == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_COLORS_APP_DARK, BM_SETCHECK, (appsUseLightTheme == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
        return TRUE;
    }
    case WM_COMMAND: {
        WORD wNotifyCode = HIWORD(wParam);
        if (wNotifyCode == BN_CLICKED || wNotifyCode == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                wchar_t highlightVal[32] = { 0 };
                GetDlgItemTextW(hwndDlg, IDC_COLORS_HIGHLIGHT, highlightVal, 32);
                DWORD classicColorsVal = (SendDlgItemMessageW(hwndDlg, IDC_COLORS_CLASSIC, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                
                RegSetValueExW(hKey, L"HighlightTint", 0, REG_SZ, (const BYTE*)highlightVal, (DWORD)(wcslen(highlightVal) + 1) * sizeof(wchar_t));
                RegSetValueExW(hKey, L"ClassicColorsEnabled", 0, REG_DWORD, (const BYTE*)&classicColorsVal, sizeof(DWORD));
                RegCloseKey(hKey);
            }

            if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
                DWORD sysUsesLightTheme = (SendDlgItemMessageW(hwndDlg, IDC_COLORS_WINDOWS_DARK, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 0 : 1;
                DWORD appsUseLightTheme = (SendDlgItemMessageW(hwndDlg, IDC_COLORS_APP_DARK, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 0 : 1;
                
                RegSetValueExW(hKey, L"SystemUsesLightTheme", 0, REG_DWORD, (const BYTE*)&sysUsesLightTheme, sizeof(DWORD));
                RegSetValueExW(hKey, L"AppsUseLightTheme", 0, REG_DWORD, (const BYTE*)&appsUseLightTheme, sizeof(DWORD));
                RegCloseKey(hKey);

                SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"ImmersiveColorSet", SMTO_ABORTIFHUNG, 500, NULL);
            }
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

INT_PTR CALLBACK GenericPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

void ShowTaskbarProperties(HWND hwndOwner) {
    std::vector<HPROPSHEETPAGE> pages;
    PROPSHEETPAGEW psp[15] = {0};
    HPROPSHEETPAGE hPage;
    
    psp[0].dwSize = sizeof(PROPSHEETPAGEW);
    psp[0].dwFlags = PSP_USETITLE;
    psp[0].hInstance = g_hInstance;
    psp[0].pszTemplate = MAKEINTRESOURCEW(IDD_TASKBAR_PROPS);
    psp[0].pfnDlgProc = TaskbarSettingsDlgProc;
    psp[0].pszTitle = L"Taskbar";
    hPage = CreatePropertySheetPageW(&psp[0]);
    if (hPage) pages.push_back(hPage);

    psp[1].dwSize = sizeof(PROPSHEETPAGEW);
    psp[1].dwFlags = PSP_USETITLE;
    psp[1].hInstance = g_hInstance;
    psp[1].pszTemplate = MAKEINTRESOURCEW(IDD_STARTMENU_PROPS);
    psp[1].pfnDlgProc = StartMenuSettingsDlgProc;
    psp[1].pszTitle = L"Start Menu";
    hPage = CreatePropertySheetPageW(&psp[1]);
    if (hPage) pages.push_back(hPage);

    psp[2].dwSize = sizeof(PROPSHEETPAGEW);
    psp[2].dwFlags = PSP_USETITLE;
    psp[2].hInstance = g_hInstance;
    psp[2].pszTemplate = MAKEINTRESOURCEW(IDD_MULTIMON_PROPS);
    psp[2].pfnDlgProc = MultiMonSettingsDlgProc;
    psp[2].pszTitle = L"Multi-Monitor Components";
    hPage = CreatePropertySheetPageW(&psp[2]);
    if (hPage) pages.push_back(hPage);

    psp[3].dwSize = sizeof(PROPSHEETPAGEW);
    psp[3].dwFlags = PSP_USETITLE;
    psp[3].hInstance = g_hInstance;
    psp[3].pszTemplate = MAKEINTRESOURCEW(IDD_NATIVE_PROPS);
    psp[3].pfnDlgProc = NativeSettingsDlgProc;
    psp[3].pszTitle = L"Native Settings";
    hPage = CreatePropertySheetPageW(&psp[3]);
    if (hPage) pages.push_back(hPage);

    psp[4].dwSize = sizeof(PROPSHEETPAGEW);
    psp[4].dwFlags = PSP_USETITLE;
    psp[4].hInstance = g_hInstance;
    psp[4].pszTemplate = MAKEINTRESOURCEW(IDD_TOOLBARS_PROPS);
    psp[4].pfnDlgProc = ToolbarsSettingsDlgProc;
    psp[4].pszTitle = L"Toolbars";
    hPage = CreatePropertySheetPageW(&psp[4]);
    if (hPage) pages.push_back(hPage);

    psp[6].dwSize = sizeof(PROPSHEETPAGEW);
    psp[6].dwFlags = PSP_USETITLE;
    psp[6].hInstance = g_hInstance;
    psp[6].pszTemplate = MAKEINTRESOURCEW(IDD_DESKTOP_PROPS);
    psp[6].pfnDlgProc = DesktopSettingsDlgProc;
    psp[6].pszTitle = L"Desktop";
    hPage = CreatePropertySheetPageW(&psp[6]);
    if (hPage) pages.push_back(hPage);

    psp[7].dwSize = sizeof(PROPSHEETPAGEW);
    psp[7].dwFlags = PSP_USETITLE;
    psp[7].hInstance = g_hInstance;
    psp[7].pszTemplate = MAKEINTRESOURCEW(IDD_EXPLORER_PROPS);
    psp[7].pfnDlgProc = ExplorerSettingsDlgProc;
    psp[7].pszTitle = L"Explorer Settings";
    hPage = CreatePropertySheetPageW(&psp[7]);
    if (hPage) pages.push_back(hPage);

    psp[8].dwSize = sizeof(PROPSHEETPAGEW);
    psp[8].dwFlags = PSP_USETITLE;
    psp[8].hInstance = g_hInstance;
    psp[8].pszTemplate = MAKEINTRESOURCEW(IDD_DWM_PROPS);
    psp[8].pfnDlgProc = DWMSettingsDlgProc;
    psp[8].pszTitle = L"DWM Settings";
    hPage = CreatePropertySheetPageW(&psp[8]);
    if (hPage) pages.push_back(hPage);

    psp[9].dwSize = sizeof(PROPSHEETPAGEW);
    psp[9].dwFlags = PSP_USETITLE;
    psp[9].hInstance = g_hInstance;
    psp[9].pszTemplate = MAKEINTRESOURCEW(IDD_COLORS_PROPS);
    psp[9].pfnDlgProc = ColorsSettingsDlgProc;
    psp[9].pszTitle = L"Colors & Themes";
    hPage = CreatePropertySheetPageW(&psp[9]);
    if (hPage) pages.push_back(hPage);


    bool showDebugTabs = false;
    if (wcsstr(GetCommandLineW(), L"/devmode")) showDebugTabs = true;
    
    HKEY hKey;
    if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD dwValue = 0, cbData = sizeof(DWORD);
        if (RegQueryValueExW(hKey, L"EnableDebugTabs", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue == 1) showDebugTabs = true;
        RegCloseKey(hKey);
    }

    if (showDebugTabs) {
        psp[5].dwSize = sizeof(PROPSHEETPAGEW);
        psp[5].dwFlags = PSP_USETITLE;
        psp[5].hInstance = g_hInstance;
        psp[5].pszTemplate = MAKEINTRESOURCEW(IDD_SECRET_EVERYTHING);
        psp[5].pfnDlgProc = GenericPageDlgProc;
        psp[5].pszTitle = L"Everything Indexer";
        hPage = CreatePropertySheetPageW(&psp[5]);
        if (hPage) pages.push_back(hPage);
    }

    if (pages.empty()) return;

    PROPSHEETHEADERW psh = { sizeof(PROPSHEETHEADERW) };
    psh.dwFlags = PSH_PROPTITLE | PSH_USEICONID | PSH_USECALLBACK;
    psh.hwndParent = hwndOwner;
    psh.hInstance = g_hInstance;
    psh.pszIcon = MAKEINTRESOURCEW(IDI_PREFERENCES);
    psh.pszCaption = L"Taskbar and Start Menu Properties";
    psh.nPages = (UINT)pages.size();
    psh.nStartPage = 0;
    psh.phpage = pages.data();
    psh.pfnCallback = PropSheetProc;

    PropertySheetW(&psh);
}

INT_PTR CALLBACK SecretDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG: return TRUE;
        case WM_COMMAND: if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) { EndDialog(hwndDlg, LOWORD(wParam)); return TRUE; } break;
    }
    return FALSE;
}

void SelectOrbComboBox(HWND hCombo, DWORD id) {
    int count = SendMessageW(hCombo, CB_GETCOUNT, 0, 0);
    for (int i = 0; i < count; i++) {
        if ((DWORD)SendMessageW(hCombo, CB_GETITEMDATA, i, 0) == id) { SendMessageW(hCombo, CB_SETCURSEL, i, 0); break; }
    }
}

DWORD GetSelectedOrbID(HWND hCombo) {
    int sel = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);
    if (sel != CB_ERR) return SendMessageW(hCombo, CB_GETITEMDATA, sel, 0);
    return IDB_START_ORB;
}

void ShowSecretEverything(HWND hwndOwner) {
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_SECRET_EVERYTHING), hwndOwner, SecretDlgProc);
}

void ShowSecretDLLScanner(HWND hwndOwner) {
    DialogBoxW(g_hInstance, MAKEINTRESOURCEW(IDD_SECRET_DLLSCANNER), hwndOwner, SecretDlgProc);
}

// Trigger build by worker_m7_gen2




