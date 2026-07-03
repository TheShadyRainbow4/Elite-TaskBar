#include "StartButton.h"
#include "Logger.h"

#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

ULONG_PTR g_gdiplusToken;
Image* g_pOrbImage = nullptr;
HWND g_hOrbWnd = NULL;
HWND g_hParentTaskbar = NULL;
OrbState g_internalOrbState = OrbState::Normal;
bool g_bOrbTracking = false;

LRESULT CALLBACK OrbWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool StartButton::Initialize(HINSTANCE hInstance, HWND hParentTaskbar) {
    GdiplusStartupInput gdiplusStartupInput;
    Status status = GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
    if (status != Ok) {
        Logger::Log(L"Failed to initialize GDI+");
        return false;
    }
    
    g_hParentTaskbar = hParentTaskbar;
    
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = OrbWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"Elite_StartOrbWnd";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);
    
    g_hOrbWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"Elite_StartOrbWnd", L"",
        WS_POPUP,
        0, 0, 54, 54,
        NULL, NULL, hInstance, NULL
    );
    
    // Force orb to always stay above start menus
    SetTimer(g_hOrbWnd, 1, 50, NULL);
    
    return true;
}

void StartButton::Cleanup() {
    if (g_hOrbWnd) DestroyWindow(g_hOrbWnd);
    if (g_pOrbImage) {
        delete g_pOrbImage;
        g_pOrbImage = nullptr;
    }
    GdiplusShutdown(g_gdiplusToken);
}

void StartButton::SetOrbImageFromResource(HINSTANCE hInstance, int resourceId) {
    if (g_pOrbImage) {
        delete g_pOrbImage;
        g_pOrbImage = nullptr;
    }

    HRSRC hResInfo = FindResourceW(hInstance, MAKEINTRESOURCEW(resourceId), (LPCWSTR)RT_RCDATA);
    if (!hResInfo) {
        Logger::Log(L"Failed to find Start Orb resource.");
        return;
    }

    HGLOBAL hResData = LoadResource(hInstance, hResInfo);
    if (!hResData) return;

    DWORD size = SizeofResource(hInstance, hResInfo);
    void* pData = LockResource(hResData);

    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hGlobal) return;

    void* pGlobalData = GlobalLock(hGlobal);
    memcpy(pGlobalData, pData, size);
    GlobalUnlock(hGlobal);

    IStream* pStream = NULL;
    if (CreateStreamOnHGlobal(hGlobal, TRUE, &pStream) == S_OK) {
        g_pOrbImage = Image::FromStream(pStream);
        pStream->Release();
    }

    if (!g_pOrbImage || g_pOrbImage->GetLastStatus() != Ok) {
        Logger::Log(L"Failed to decode Start Orb image stream.");
    }
}

void StartButton::Draw() {
    if (!g_pOrbImage || !g_hOrbWnd || g_pOrbImage->GetLastStatus() != Ok) {
        return;
    }

    UINT imgWidth = g_pOrbImage->GetWidth();
    UINT sliceHeight = g_pOrbImage->GetHeight() / 3;

    int srcY = 0;
    switch (g_internalOrbState) {
        case OrbState::Normal:  srcY = 0; break;
        case OrbState::Hover:   srcY = sliceHeight; break;
        case OrbState::Pressed: srcY = sliceHeight * 2; break;
    }

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = imgWidth;
    bmi.bmiHeader.biHeight = -(LONG)sliceHeight; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* pBits = nullptr;
    HBITMAP hbmMem = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);

    {
        Graphics graphics(hdcMem);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        // Clear the background to fully transparent
        graphics.Clear(Color(0, 0, 0, 0));

        Rect destRect(0, 0, imgWidth, sliceHeight);
        graphics.DrawImage(g_pOrbImage, destRect, 0, srcY, imgWidth, sliceHeight, UnitPixel);
    }

    POINT ptSrc = {0, 0};
    SIZE size = { (LONG)imgWidth, (LONG)sliceHeight };
    BLENDFUNCTION blend = {0};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA; // Per-pixel alpha!

    UpdateLayeredWindow(g_hOrbWnd, hdcScreen, NULL, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void StartButton::Show(int taskbarX, int taskbarY, int taskbarHeight) {
    if (!g_pOrbImage || !g_hOrbWnd) return;
    UINT imgWidth = g_pOrbImage->GetWidth();
    UINT sliceHeight = g_pOrbImage->GetHeight() / 3;
    
    // Center vertically on the taskbar. This creates the classic Windows 7 effect where 
    // the top overhangs onto the desktop, and the bottom gets cropped by the screen edge.
    int yPos = taskbarY + (taskbarHeight - (int)sliceHeight) / 2;
    SetWindowPos(g_hOrbWnd, HWND_TOPMOST, taskbarX, yPos, imgWidth, sliceHeight, SWP_SHOWWINDOW);
    Draw();
}

LRESULT CALLBACK OrbWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_MOUSEMOVE: {
            if (!g_bOrbTracking) {
                TRACKMOUSEEVENT tme = {0};
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                g_bOrbTracking = true;
            }
            if (g_internalOrbState != OrbState::Pressed) {
                if (g_internalOrbState != OrbState::Hover) {
                    g_internalOrbState = OrbState::Hover;
                    StartButton::Draw();
                }
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            g_bOrbTracking = false;
            if (g_internalOrbState != OrbState::Normal) {
                g_internalOrbState = OrbState::Normal;
                StartButton::Draw();
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            g_internalOrbState = OrbState::Pressed;
            StartButton::Draw();
            return 0;
        }
        case WM_LBUTTONUP: {
            if (g_internalOrbState == OrbState::Pressed) {
                g_internalOrbState = OrbState::Hover;
                StartButton::Draw();
                
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                
                DWORD mode = 0;
                HKEY hKey;
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    DWORD cbData = sizeof(DWORD);
                    RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&mode, &cbData);
                    RegCloseKey(hKey);
                }

                bool isShiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

                if (mode == 1) { // Native
                    SendMessageW(FindWindowW(L"Shell_TrayWnd", NULL), WM_SYSCOMMAND, SC_TASKLIST, 0);
                } else if (mode == 2) { // Combo
                    if (isShiftDown) {
                        SendMessageW(FindWindowW(L"Shell_TrayWnd", NULL), WM_SYSCOMMAND, SC_TASKLIST, 0);
                    } else {
                        keybd_event(VK_LWIN, 0, 0, 0);
                        keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
                    }
                } else { // OpenShell (Default 0)
                    keybd_event(VK_LWIN, 0, 0, 0);
                    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
                }
            }
            return 0;
        }
        case WM_TIMER: {
            if (wParam == 1) {
                // Ensure orb stays above OpenShell / Native Start Menu
                SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            return 0;
        }
        case WM_RBUTTONUP: {
            if (g_hParentTaskbar) {
                // Trick the taskbar into thinking we clicked at x=0 so it opens the start menu context
                SendMessageW(g_hParentTaskbar, WM_RBUTTONUP, wParam, MAKELPARAM(0, 0));
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
