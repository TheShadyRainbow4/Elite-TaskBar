// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DarkModeManager.h"
#include "Config.h"
#include "EventWindow.h"

DarkModeManager::DarkModeManager(EventWindow * /*eventWindow*/, const Config *config) : m_config(config)
{
	/* Dark mode support has been removed. */
	m_darkModeSupported = false;
	m_darkModeEnabled = false;
}

bool DarkModeManager::IsDarkModeSupported() const
{
	return false;
}

bool DarkModeManager::IsDarkModeEnabled() const
{
	return false;
}

void DarkModeManager::AllowDarkModeForWindow(HWND /*hwnd*/, bool /*allow*/)
{
}

bool DarkModeManager::IsSystemAppModeLight() const
{
	return true;
}

bool DarkModeManager::IsHighContrast()
{
	HIGHCONTRASTW hc = { sizeof(hc) };
	if (SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(hc), &hc, 0))
	{
		return (hc.dwFlags & HCF_HIGHCONTRASTON) != 0;
	}
	return false;
}

void DarkModeManager::OnEventWindowMessage(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
}

void DarkModeManager::OnSettingChange(const wchar_t * /*systemParameter*/)
{
}

void DarkModeManager::OnThemeUpdated()
{
}

void DarkModeManager::UpdateAppDarkModeStatus()
{
}

bool DarkModeManager::ShouldEnableDarkMode() const
{
	return false;
}

void DarkModeManager::AllowDarkModeForApp(bool /*allow*/)
{
}

void DarkModeManager::FlushMenuThemes()
{
}

void DarkModeManager::RefreshImmersiveColorPolicyState()
{
}

LONG DarkModeManager::DetourOpenNcThemeData()
{
	return 0;
}

LONG DarkModeManager::RestoreOpenNcThemeData()
{
	return 0;
}

HTHEME WINAPI DarkModeManager::DetouredOpenNcThemeData(HWND hwnd, LPCWSTR classList)
{
	return OpenThemeData(hwnd, classList);
}
