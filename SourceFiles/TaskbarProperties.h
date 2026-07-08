#pragma once
#include <windows.h>

void ShowTaskbarProperties(HWND hwndOwner);
void ShowSecretEverything(HWND hwndOwner);
void ShowSecretDLLScanner(HWND hwndOwner);
void ImportSettingsFromXMLPathSilently(const wchar_t* xmlPath);
void NotifySettingsChange();
