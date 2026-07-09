#pragma once
#include <string>
#include <windows.h>

namespace Logger {
    void Initialize();
    void Log(const std::wstring& message);
    void LogError(const std::wstring& message, DWORD errorCode);
}

// Global MessageBoxW override to rewrite OK to Okay - Builder-Bob
int EliteMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
#ifdef MessageBoxW
#undef MessageBoxW
#endif
#define MessageBoxW EliteMessageBoxW

