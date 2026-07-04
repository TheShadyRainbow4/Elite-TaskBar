#include "stdafx.h"
#pragma warning(disable: 4100 4244 4267 4189 4312)
#include "Logger.h"
#include <windows.h>
#include <fstream>
#include <shlobj.h>
#include <sstream>
#include <iomanip>

std::wstring GetLogFilePath() {
    wchar_t systemDrive[MAX_PATH];
    DWORD res = GetEnvironmentVariableW(L"SystemDrive", systemDrive, MAX_PATH);
    if (res == 0) {
        wcscpy_s(systemDrive, MAX_PATH, L"C:");
    }
    
    std::wstring logDir = std::wstring(systemDrive) + L"\\EliteSoftware\\Logs";
    
    // Ensure directories exist
    CreateDirectoryW((std::wstring(systemDrive) + L"\\EliteSoftware").c_str(), NULL);
    CreateDirectoryW(logDir.c_str(), NULL);
    
    return logDir + L"\\EliteTaskbar.log";
}

void Logger::Initialize() {
    Log(L"--- EliteTaskbar Bootstrapper Initiated ---");
}

void Logger::Log(const std::wstring& message) {
    SYSTEMTIME st;
    GetLocalTime(&st);

    std::wstringstream ss;
    ss << L"[" << std::setfill(L'0') << std::setw(4) << st.wYear << L"-"
       << std::setw(2) << st.wMonth << L"-"
       << std::setw(2) << st.wDay << L" "
       << std::setw(2) << st.wHour << L":"
       << std::setw(2) << st.wMinute << L":"
       << std::setw(2) << st.wSecond << L"] "
       << message << L"\n";

    std::wstring logPath = GetLogFilePath();
    std::wofstream file(logPath.c_str(), std::ios_base::app);
    if (file.is_open()) {
        file << ss.str();
    } else {
        wchar_t msg[512];
        wsprintfW(msg, L"Failed to open log file:\n%s", logPath.c_str());
        MessageBoxW(NULL, msg, L"EliteTaskbar - Logger Error", MB_ICONERROR | MB_OK);
    }
    OutputDebugStringW(ss.str().c_str());
}

void Logger::LogError(const std::wstring& message, DWORD errorCode) {
    std::wstringstream ss;
    ss << message << L" (Hex Error Code: 0x" << std::hex << errorCode << L")";
    Log(ss.str());
}



