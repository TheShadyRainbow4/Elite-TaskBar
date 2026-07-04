#include "stdafx.h"
#pragma warning(disable: 4100 4244 4267 4189 4312)
#include "StartButton.h"
#include "Logger.h"
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#pragma comment (lib,"Gdiplus.lib")

using namespace Gdiplus;

LRESULT CALLBACK OrbWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

bool StartButton::GlobalInitialize(ULONG_PTR& token) {
    GdiplusStartupInput gdiplusStartupInput;
    Status status = GdiplusStartup(&token, &gdiplusStartupInput, NULL);
    if (status != Ok) {
        Logger::Log(L"Failed to initialize GDI+");
        return false;
    }
    return true;
}

void StartButton::GlobalCleanup(ULONG_PTR token) {
    GdiplusShutdown(token);
}

StartButton::StartButton() : m_pOrbImage(nullptr), m_hOrbWnd(NULL), m_hParentTaskbar(NULL), m_monitorIndex(-1), m_internalOrbState(OrbState::Normal), m_bOrbTracking(false) {}

StartButton::~StartButton() {
    if (m_hOrbWnd) DestroyWindow(m_hOrbWnd);
    if (m_pOrbImage) delete m_pOrbImage;
}

bool StartButton::Initialize(HINSTANCE hInstance, HWND hParentTaskbar, int monitorIndex) {
    m_hParentTaskbar = hParentTaskbar;
    m_monitorIndex = monitorIndex;
    
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = OrbWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"Elite_StartOrbWnd";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // Ignore register errors if class already exists
    RegisterClassExW(&wc);
    
    m_hOrbWnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        L"Elite_StartOrbWnd", L"",
        WS_POPUP,
        0, 0, 54, 54,
        NULL, NULL, hInstance, this
    );
    
    if (m_hOrbWnd) {
        SetWindowLongPtr(m_hOrbWnd, GWLP_USERDATA, (LONG_PTR)this);
        SetTimer(m_hOrbWnd, 1, 50, NULL);
    }
    
    return m_hOrbWnd != NULL;
}

void StartButton::SetOrbImageFromResource(HINSTANCE hInstance, int resourceId) {
    if (m_pOrbImage) {
        delete m_pOrbImage;
        m_pOrbImage = nullptr;
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
        m_pOrbImage = Image::FromStream(pStream);
        pStream->Release();
    }

    if (!m_pOrbImage || m_pOrbImage->GetLastStatus() != Ok) {
        Logger::Log(L"Failed to decode Start Orb image stream.");
    }
}

void StartButton::ReloadOrbImage(HINSTANCE hInstance, int monitorIndex) {
    DWORD orbId = 103; // IDB_START_ORB
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD cbData = sizeof(DWORD);
        
        bool found = false;
        if (monitorIndex >= 0) {
            WCHAR valueName[64];
            wsprintfW(valueName, L"StartOrbID_Mon%d", monitorIndex);
            if (RegQueryValueExW(hKey, valueName, NULL, NULL, (LPBYTE)&orbId, &cbData) == ERROR_SUCCESS) {
                found = true;
            }
        }
        
        if (!found) {
            RegQueryValueExW(hKey, L"StartOrbID", NULL, NULL, (LPBYTE)&orbId, &cbData);
        }
        
        RegCloseKey(hKey);
    }
    SetOrbImageFromResource(hInstance, (int)orbId);
}

void StartButton::Draw() {
    if (!m_pOrbImage || !m_hOrbWnd || m_pOrbImage->GetLastStatus() != Ok) {
        return;
    }

    UINT imgWidth = m_pOrbImage->GetWidth();
    UINT imgHeight = m_pOrbImage->GetHeight();
    int numFrames = ((imgHeight * 100) / imgWidth > 300) ? 4 : 3;
    UINT sliceHeight = imgHeight / numFrames;

    int srcY = 0;
    switch (m_internalOrbState) {
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
        graphics.DrawImage(m_pOrbImage, destRect, 0, srcY, imgWidth, sliceHeight, UnitPixel);
    }

    POINT ptSrc = {0, 0};
    SIZE size = { (LONG)imgWidth, (LONG)sliceHeight };
    BLENDFUNCTION blend = {0};
    blend.BlendOp = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA; // Per-pixel alpha!

    UpdateLayeredWindow(m_hOrbWnd, hdcScreen, NULL, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hbmOld);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void StartButton::Show(int taskbarX, int taskbarY, int taskbarHeight) {
    if (!m_pOrbImage || !m_hOrbWnd) return;
    UINT imgWidth = m_pOrbImage->GetWidth();
    UINT imgHeight = m_pOrbImage->GetHeight();
    int numFrames = ((imgHeight * 100) / imgWidth > 300) ? 4 : 3;
    UINT sliceHeight = imgHeight / numFrames;
    
    int yPos = taskbarY + (taskbarHeight - (int)sliceHeight) / 2;
    SetWindowPos(m_hOrbWnd, HWND_TOPMOST, taskbarX, yPos, imgWidth, sliceHeight, SWP_SHOWWINDOW);
    Draw();
}

LRESULT CALLBACK OrbWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    StartButton* pThis = (StartButton*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (!pThis) return DefWindowProcW(hwnd, uMsg, wParam, lParam);

    switch (uMsg) {
        case WM_MOUSEMOVE: {
            if (!pThis->IsTracking()) {
                TRACKMOUSEEVENT tme = {0};
                tme.cbSize = sizeof(TRACKMOUSEEVENT);
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = hwnd;
                TrackMouseEvent(&tme);
                pThis->SetTracking(true);
            }
            if (pThis->GetState() != OrbState::Pressed) {
                if (pThis->GetState() != OrbState::Hover) {
                    pThis->SetState(OrbState::Hover);
                    pThis->Draw();
                }
            }
            return 0;
        }
        case WM_MOUSELEAVE: {
            pThis->SetTracking(false);
            if (pThis->GetState() != OrbState::Normal) {
                pThis->SetState(OrbState::Normal);
                pThis->Draw();
            }
            return 0;
        }
        case WM_LBUTTONDOWN: {
            pThis->SetState(OrbState::Pressed);
            pThis->Draw();
            return 0;
        }
        case WM_LBUTTONUP: {
            if (pThis->GetState() == OrbState::Pressed) {
                pThis->SetState(OrbState::Hover);
                pThis->Draw();
                
                // SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE); // Removed to fix flashing
                
                DWORD mode = 0;
                HKEY hKey;
                if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                    DWORD cbData = sizeof(DWORD);
                    wchar_t modeKey[32];
                    swprintf_s(modeKey, L"StartMenuMode_Mon%d", pThis->GetMonitorIndex());
                    if (RegQueryValueExW(hKey, modeKey, NULL, NULL, (LPBYTE)&mode, &cbData) != ERROR_SUCCESS) {
                        // Fallback to global mode if monitor-specific mode not set
                        cbData = sizeof(DWORD);
                        RegQueryValueExW(hKey, L"StartMenuMode", NULL, NULL, (LPBYTE)&mode, &cbData);
                    }
                    RegCloseKey(hKey);
                }

                bool isShiftDown = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

                HWND hNativeTarget = NULL;
                HMONITOR hMon = MonitorFromWindow(pThis->GetParentTaskbar(), MONITOR_DEFAULTTONULL);
                if (hMon) {
                    HWND hPrimary = FindWindowW(L"Shell_TrayWnd", NULL);
                    if (MonitorFromWindow(hPrimary, MONITOR_DEFAULTTONULL) == hMon) {
                        hNativeTarget = hPrimary;
                    } else {
                        HWND hSec = NULL;
                        while ((hSec = FindWindowExW(NULL, hSec, L"Shell_SecondaryTrayWnd", NULL)) != NULL) {
                            if (MonitorFromWindow(hSec, MONITOR_DEFAULTTONULL) == hMon) {
                                hNativeTarget = hSec;
                                break;
                            }
                        }
                    }
                }
                if (!hNativeTarget) hNativeTarget = FindWindowW(L"Shell_TrayWnd", NULL);
                if (!hNativeTarget) hNativeTarget = pThis->GetParentTaskbar(); // Fallback for PE without native shell

                POINT pt;
                GetCursorPos(&pt);
                LPARAM lCursorParam = MAKELPARAM(pt.x, pt.y);
                
                if (mode == 0) {
                    // Open-Shell Menu
                    bool injectShift = isShiftDown;
                    if (injectShift) keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
                    SendMessageW(hNativeTarget, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(2, 2));
                    SendMessageW(hNativeTarget, WM_LBUTTONUP, 0, MAKELPARAM(2, 2));
                    if (injectShift) keybd_event(VK_SHIFT, 0, 0, 0);
                } else if (mode == 1 || (mode == 3 && !isShiftDown) || (mode == 2 && isShiftDown)) {
                    // Open Native Menu (Requires Shift for Open-Shell users)
                    bool injectShift = !isShiftDown;
                    if (injectShift) keybd_event(VK_SHIFT, 0, 0, 0);
                    SendMessageW(hNativeTarget, WM_SYSCOMMAND, SC_TASKLIST, lCursorParam);
                    if (injectShift) keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
                } else {
                    // Elite Custom Menu (Open-Shell Mirror)
                    WCHAR path[MAX_PATH];
                    GetModuleFileNameW(NULL, path, MAX_PATH);
                    PathRemoveFileSpecW(path);
                    PathAppendW(path, L"EliteStartMenu.exe");
                    
                    WCHAR args[256];
                    wsprintfW(args, L"\"%s\" -MonitorIndex %d -TaskbarY %d", path, pThis->GetMonitorIndex(), pt.y);
                    
                    STARTUPINFOW si = { sizeof(si) };
                    PROCESS_INFORMATION pi;
                    CreateProcessW(path, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
                    if (pi.hProcess) {
                        CloseHandle(pi.hProcess);
                        CloseHandle(pi.hThread);
                    }
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
            HWND hParent = pThis->GetParentTaskbar();
            if (hParent) {
                SendMessageW(hParent, WM_RBUTTONUP, wParam, MAKELPARAM(0, 0));
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}



