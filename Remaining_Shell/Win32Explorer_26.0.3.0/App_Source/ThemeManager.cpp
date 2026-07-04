// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ThemeManager.h"
#include "../Shared_Libraries/WindowSubclass.h"

ThemeManager::ThemeManager(DarkModeManager * /*darkModeManager*/,
	const DarkModeColorProvider * /*darkModeColorProvider*/) :
	m_darkModeManager(nullptr),
	m_darkModeColorProvider(nullptr)
{
}

void ThemeManager::TrackTopLevelWindow(HWND /*hwnd*/)
{
}

void ThemeManager::UntrackTopLevelWindow(HWND /*hwnd*/)
{
}

void ThemeManager::ApplyThemeToWindowAndChildren(HWND /*hwnd*/)
{
}

void ThemeManager::OnDarkModeStatusChanged()
{
}

void ThemeManager::ApplyThemeToWindow(HWND /*hwnd*/)
{
}

BOOL ThemeManager::ProcessChildWindow(HWND /*hwnd*/)
{
	return TRUE;
}

BOOL ThemeManager::ProcessThreadWindow(HWND /*hwnd*/)
{
	return TRUE;
}

void ThemeManager::ApplyThemeToMainWindow(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToDialog(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToTabControl(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToListView(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToHeader(HWND /*hwnd*/)
{
}

void ThemeManager::ApplyThemeToTreeView(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToRichEdit(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToRebar(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToToolbar(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToComboBoxEx(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToComboBox(HWND /*hwnd*/)
{
}

void ThemeManager::ApplyThemeToEditControl(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToButton(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToTooltips(HWND /*hwnd*/)
{
}

void ThemeManager::ApplyThemeToStatusBar(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToScrollBar(HWND /*hwnd*/, bool /*enableDarkMode*/)
{
}

void ThemeManager::ApplyThemeToUpDownControl(HWND /*hwnd*/)
{
}

LRESULT ThemeManager::MainWindowSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

HBRUSH ThemeManager::GetMenuBarBackgroundBrush(bool /*enableDarkMode*/)
{
	return nullptr;
}

bool ThemeManager::ShouldAlwaysShowAccessKeys()
{
	return false;
}

LRESULT ThemeManager::DialogSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT ThemeManager::ToolbarParentSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT ThemeManager::OnCustomDraw(NMCUSTOMDRAW * /*customDraw*/)
{
	return CDRF_DODEFAULT;
}

LRESULT ThemeManager::OnButtonCustomDraw(NMCUSTOMDRAW * /*customDraw*/)
{
	return CDRF_DODEFAULT;
}

LRESULT ThemeManager::OnToolbarCustomDraw(NMTBCUSTOMDRAW * /*customDraw*/)
{
	return CDRF_DODEFAULT;
}

LRESULT ThemeManager::ComboBoxExSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT ThemeManager::TabControlSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT ThemeManager::ListViewSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT ThemeManager::RebarSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT ThemeManager::GroupBoxSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}

LRESULT ThemeManager::ScrollBarSubclass(HWND /*hwnd*/, UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	return 0;
}
