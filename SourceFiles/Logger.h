#pragma once
#include <string>
#include <windows.h>

namespace Logger {
    void Initialize();
    void Log(const std::wstring& message);
    void LogError(const std::wstring& message, DWORD errorCode);
}
