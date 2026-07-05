#pragma warning(disable: 4996)
#include "TaskbarWindow.h"
#include "resource.h"
extern HINSTANCE g_hInstance;
#include "Config.h"
#include "Logger.h"
#include "StartButton.h"
#include <commctrl.h>
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
            rcChin = { 0, 168, 250, 195 };
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
            if (bExpanded) {
                SetDlgItemTextW(hwndDlg, IDC_ABOUT_EXPAND, L"Less Info <<");
                
                RECT rcDlg = { 0, 0, 250, 195 };
                MapDialogRect(hwndDlg, &rcDlg);
                SetWindowPos(hwndDlg, NULL, 0, 0, rcDlg.right, rcDlg.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
                
                ShowWindow(GetDlgItem(hwndDlg, IDC_ABOUT_MOREINFO), SW_SHOW);
                
                RECT rcExpand = { 10, 172, 70, 186 };
                MapDialogRect(hwndDlg, &rcExpand);
                SetWindowPos(GetDlgItem(hwndDlg, IDC_ABOUT_EXPAND), NULL, rcExpand.left, rcExpand.top, rcExpand.right - rcExpand.left, rcExpand.bottom - rcExpand.top, SWP_NOZORDER | SWP_NOACTIVATE);
                
                RECT rcOk = { 190, 172, 240, 186 };
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
                SetWindowPos(hwndDlg, NULL, 0, 0, rcDlg.right, rcDlg.bottom, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
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
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
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
    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\openinWin32Explorer");
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        DWORD cbData = sizeof(WCHAR) * 40;
        WCHAR val[40] = {0};
        if (RegQueryValueExW(hKey, L"", NULL, NULL, (LPBYTE)val, &cbData) == ERROR_SUCCESS) {
            if (wcscmp(val, L"openinWin32Explorer") == 0) {
                RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"none", sizeof(L"none"));
            }
        }
        RegCloseKey(hKey);
    }

    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Folder\\shell\\openinWin32Explorer");
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Folder\\shell", 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
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
        
        if (RegCreateKeyExW(HKEY_CURRENT_USER, commandKeyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
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
        if (RegOpenKeyExW(HKEY_CURRENT_USER, rootSubKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"openinWin32Explorer", sizeof(L"openinWin32Explorer"));
            RegCloseKey(hKey);
        }
    }
}

DWORD WINAPI BroadcastSettingsChangeThread(LPVOID lpParam) {
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings", SMTO_ABORTIFHUNG, 5000, NULL);
    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"EliteTaskbarSettings", SMTO_ABORTIFHUNG, 500, NULL);
    
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
    swprintf_s(psCmd, L"-NoProfile -WindowStyle Hidden -Command \"Stop-Process -Name EliteTaskbar -Force; Stop-Process -Name Win32Explorer -Force; Start-Sleep -Milliseconds 500; Start-Process -FilePath '%s\\EliteTaskbar.exe' -ErrorAction SilentlyContinue; Start-Process -FilePath '%s\\Win32Explorer.exe' -ErrorAction SilentlyContinue\"", exePath, exePath);

    ShellExecuteW(NULL, NULL, L"powershell.exe", psCmd, NULL, SW_HIDE);
        
    return 0;
}

void NotifySettingsChange() {
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
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKeyBoth, NULL) == ERROR_SUCCESS) {
                RegSetValueExW(hKeyBoth, L"EnablePortableMirror", 0, REG_DWORD, (const BYTE*)&portable, sizeof(DWORD));
                RegCloseKey(hKeyBoth);
            }

            HKEY hKey;
            HKEY hKeyRoot = (portable == 1) ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
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
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            DWORD dwValue = 0, cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarSizeMove", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_SETCHECK, (dwValue == 0) ? BST_CHECKED : BST_UNCHECKED, 0);
                
            cbData = sizeof(DWORD);
            if (RegQueryValueExW(hKey, L"TaskbarSmallIcons", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS)
                SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
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

                    WriteKey(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", L"HKEY_CURRENT_USER\\Software\\EliteSoftware\\Win32Explorer\\Advanced");
                    WriteVal(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarSizeMove");
                    WriteVal(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", L"TaskbarSmallIcons");
                    WriteVal(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", L"HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", L"Settings");
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
            
            DWORD locked = (SendDlgItemMessageW(hwndDlg, IDC_LOCK_TASKBAR, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 0 : 1;
            SaveToNativeRegistry(L"TaskbarSizeMove", locked);
            DWORD smallIcons = (SendDlgItemMessageW(hwndDlg, IDC_SMALL_ICONS, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
            SaveToNativeRegistry(L"TaskbarSmallIcons", smallIcons);
            
            if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\StuckRects3", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
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
    RECT rc;
    GetClientRect(hwndDlg, &rc);
    HWND hScroll = CreateWindowExW(WS_EX_CONTROLPARENT, L"EliteDynScrollArea", L"", WS_CHILD | WS_VISIBLE | WS_VSCROLL, 0, 0, rc.right, rc.bottom, hwndDlg, NULL, g_hInstance, NULL);
    if (hPlaceholder) DestroyWindow(hPlaceholder);
    return hScroll;
}

struct MonitorInfo {
    int index;
    HMONITOR hMonitor;
    RECT rect;
};
std::vector<MonitorInfo> g_Monitors;
BOOL CALLBACK TaskbarPropsMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
    MonitorInfo info;
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
        RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey);
        
        for (const auto& mon : g_Monitors) {
            WCHAR title[64];
            wsprintfW(title, L"Monitor %d (%dx%d)", mon.index, mon.rect.right - mon.rect.left, mon.rect.bottom - mon.rect.top);
            HWND hGroup = CreateWindowExW(0, L"Button", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX, 5, y, 320, 175, hScroll, NULL, g_hInstance, NULL);
            SendMessageW(hGroup, WM_SETFONT, (WPARAM)hFont, 0);
            
            HWND hChk1 = CreateWindowExW(0, L"Button", L"System Tray", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TRAY + mon.index), g_hInstance, NULL);
            HWND hChk2 = CreateWindowExW(0, L"Button", L"Clock", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 120, y + 20, 100, 15, hScroll, (HMENU)(ID_BASE_MM_CLOCK + mon.index), g_hInstance, NULL);
            HWND hChk3 = CreateWindowExW(0, L"Button", L"Task Buttons", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP, 15, y + 40, 100, 15, hScroll, (HMENU)(ID_BASE_MM_TBTN + mon.index), g_hInstance, NULL);
            
            SendMessageW(hChk1, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk2, WM_SETFONT, (WPARAM)hFont, 0);
            SendMessageW(hChk3, WM_SETFONT, (WPARAM)hFont, 0);
            
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

            AddTooltip(hScroll, hChk1, L"Show system tray icons on this monitor. Notification overload!");
            AddTooltip(hScroll, hChk2, L"Show clock widget on this monitor. Never lose track of time!");
            AddTooltip(hScroll, hChk3, L"Show application task buttons on this monitor. Keep tabs on everything.");
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
                DWORD dwValue = 0, cbData = sizeof(DWORD);
                WCHAR val[64];
                wsprintfW(val, L"EnableTray_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                else if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                
                wsprintfW(val, L"EnableClock_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                else SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                
                wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
                if (RegQueryValueExW(hKey, val, NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue) SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
                else SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);

                DWORD mode = 0; cbData = sizeof(DWORD);
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
                if (mon.index == 0) SendMessageW(hChk1, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk2, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hChk3, BM_SETCHECK, BST_CHECKED, 0);
                SendMessageW(hCmbMode, CB_SETCURSEL, 0, 0);
                SendMessageW(hCmbTrig, CB_SETCURSEL, 0, 0);
            }
            
            y += 185;
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
            if (RegCreateKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                for (const auto& mon : g_Monitors) {
                    DWORD v1 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_TRAY + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v2 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_CLOCK + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    DWORD v3 = (SendMessageW(GetDlgItem(hScroll, ID_BASE_MM_TBTN + mon.index), BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                    
                    DWORD mode = SendMessageW(GetDlgItem(hScroll, ID_BASE_SM_MODE + mon.index), CB_GETCURSEL, 0, 0);
                    DWORD trig = SendMessageW(GetDlgItem(hScroll, ID_BASE_SM_TRIG + mon.index), CB_GETCURSEL, 0, 0);
                    DWORD orb = GetSelectedOrbID(GetDlgItem(hScroll, ID_BASE_SM_ORB + mon.index));

                    WCHAR val[64];
                    wsprintfW(val, L"EnableTray_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v1, sizeof(DWORD));
                    wsprintfW(val, L"EnableClock_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v2, sizeof(DWORD));
                    wsprintfW(val, L"EnableTaskBtns_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&v3, sizeof(DWORD));
                    
                    wsprintfW(val, L"StartMenuMode_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&mode, sizeof(DWORD));
                    wsprintfW(val, L"StartMenuTrigger_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&trig, sizeof(DWORD));
                    wsprintfW(val, L"StartOrbID_Mon%d", mon.index);
                    RegSetValueExW(hKey, val, 0, REG_DWORD, (const BYTE*)&orb, sizeof(DWORD));
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
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        AddDlgTooltip(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, L"Enable Open-Shell integration fallback when in replace mode. Classic Start Menu experience.");
        
        CreateDynScrollArea(hwndDlg, IDC_DYN_SCROLLAREA);
        
        HKEY hKey;
        DWORD dwValue = 1; // Default to 1
        DWORD cbData = sizeof(DWORD);
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegQueryValueExW(hKey, L"FallbackStartMenuEnabled", NULL, NULL, (LPBYTE)&dwValue, &cbData);
            RegCloseKey(hKey);
        }
        SendDlgItemMessageW(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, BM_SETCHECK, dwValue ? BST_CHECKED : BST_UNCHECKED, 0);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            HKEY hKeyRoot = GetEliteRegistryRoot();
            if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD fallbackVal = (SendDlgItemMessageW(hwndDlg, IDC_FALLBACK_STARTMENU_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                RegSetValueExW(hKey, L"FallbackStartMenuEnabled", 0, REG_DWORD, (const BYTE*)&fallbackVal, sizeof(DWORD));
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

INT_PTR CALLBACK DesktopSettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_INITDIALOG: {
        EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, L"Enable custom desktop replacement window. Pure classic shell experience.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, L"Render desktop background wallpaper using custom styles.");
        AddDlgTooltip(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, L"Display folder items and files on the desktop grid.");
        
        HKEY hKey;
        DWORD replaceVal = 1;
        DWORD wallpaperVal = 1;
        DWORD iconsVal = 1;
        DWORD cbData = sizeof(DWORD);
        
        if (RegOpenKeyExW(GetEliteRegistryRoot(), L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopReplacementEnabled", NULL, NULL, (LPBYTE)&replaceVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopWallpaperEnabled", NULL, NULL, (LPBYTE)&wallpaperVal, &cbData);
            cbData = sizeof(DWORD);
            RegQueryValueExW(hKey, L"DesktopIconsEnabled", NULL, NULL, (LPBYTE)&iconsVal, &cbData);
            RegCloseKey(hKey);
        }
        
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, BM_SETCHECK, replaceVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_SETCHECK, wallpaperVal ? BST_CHECKED : BST_UNCHECKED, 0);
        SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, BM_SETCHECK, iconsVal ? BST_CHECKED : BST_UNCHECKED, 0);
        return TRUE;
    }
    case WM_COMMAND:
        if (HIWORD(wParam) == BN_CLICKED || HIWORD(wParam) == CBN_SELCHANGE || HIWORD(wParam) == EN_CHANGE) {
            SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
        }
        return TRUE;
    case WM_NOTIFY: {
        LPNMHDR lpnm = (LPNMHDR)lParam;
        if (lpnm->code == PSN_APPLY) {
            HKEY hKey;
            HKEY hKeyRoot = GetEliteRegistryRoot();
            if (RegCreateKeyExW(hKeyRoot, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
                DWORD replaceVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_REPLACE_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD wallpaperVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_WALLPAPER_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                DWORD iconsVal = (SendDlgItemMessageW(hwndDlg, IDC_DESKTOP_ICONS_ENABLED, BM_GETCHECK, 0, 0) == BST_CHECKED) ? 1 : 0;
                
                RegSetValueExW(hKey, L"DesktopReplacementEnabled", 0, REG_DWORD, (const BYTE*)&replaceVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopWallpaperEnabled", 0, REG_DWORD, (const BYTE*)&wallpaperVal, sizeof(DWORD));
                RegSetValueExW(hKey, L"DesktopIconsEnabled", 0, REG_DWORD, (const BYTE*)&iconsVal, sizeof(DWORD));
                
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

INT_PTR CALLBACK GenericPageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

void ShowTaskbarProperties(HWND hwndOwner) {
    std::vector<HPROPSHEETPAGE> pages;
    PROPSHEETPAGEW psp[10] = {0};
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




