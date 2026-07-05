#pragma once
#include <windows.h>

void InitializeTaskbar(HINSTANCE hInstance);
void CleanupTaskbar();
bool IsEliteTaskbarRunning();
