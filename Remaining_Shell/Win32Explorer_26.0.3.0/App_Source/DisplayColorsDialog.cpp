// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "DisplayColorsDialog.h"
#include "Config.h"
#include "MainResource.h"
#include "ResourceLoader.h"
#include "../Shared_Libraries/WindowHelper.h"
#include <Commctrl.h>
#include <string>

DisplayColorsDialog *DisplayColorsDialog::Create(const ResourceLoader *resourceLoader, HWND hParent, Config *config)
{
	return new DisplayColorsDialog(resourceLoader, hParent, config);
}

DisplayColorsDialog::DisplayColorsDialog(const ResourceLoader *resourceLoader, HWND hParent, Config *config) :
	BaseDialog(resourceLoader, IDD_DISPLAYCOLOURS, hParent, DialogSizingType::None),
	m_config(config)
{
}

INT_PTR DisplayColorsDialog::OnInitDialog()
{
	ApplyColorsToSliders();
	CenterWindow(GetParent(m_hDlg), m_hDlg);
	return TRUE;
}

void DisplayColorsDialog::ApplyColorsToSliders()
{
	COLORREF centreColor = m_config->displayWindowCentreColor.get();
	COLORREF surroundColor = m_config->displayWindowSurroundColor.get();

	int centreRed = GetRValue(centreColor);
	int centreGreen = GetGValue(centreColor);
	int centreBlue = GetBValue(centreColor);

	int surroundRed = GetRValue(surroundColor);
	int surroundGreen = GetGValue(surroundColor);
	int surroundBlue = GetBValue(surroundColor);

	auto initSlider = [this](int sliderId, int editId, int value)
	{
		HWND hSlider = GetDlgItem(m_hDlg, sliderId);
		SendMessage(hSlider, TBM_SETRANGE, TRUE, MAKELPARAM(0, 255));
		SendMessage(hSlider, TBM_SETPOS, TRUE, value);
		SetDlgItemInt(m_hDlg, editId, value, FALSE);
	};

	initSlider(IDC_SLIDER_CENTRE_RED, IDC_EDIT_CENTRE_RED, centreRed);
	initSlider(IDC_SLIDER_CENTRE_GREEN, IDC_EDIT_CENTRE_GREEN, centreGreen);
	initSlider(IDC_SLIDER_CENTRE_BLUE, IDC_EDIT_CENTRE_BLUE, centreBlue);

	initSlider(IDC_SLIDER_SURROUND_RED, IDC_EDIT_SURROUND_RED, surroundRed);
	initSlider(IDC_SLIDER_SURROUND_GREEN, IDC_EDIT_SURROUND_GREEN, surroundGreen);
	initSlider(IDC_SLIDER_SURROUND_BLUE, IDC_EDIT_SURROUND_BLUE, surroundBlue);
}

void DisplayColorsDialog::SyncSliderToEdit(HWND hwndSlider, HWND hwndEdit)
{
	int pos = static_cast<int>(SendMessage(hwndSlider, TBM_GETPOS, 0, 0));
	SetWindowText(hwndEdit, std::to_wstring(pos).c_str());
}

void DisplayColorsDialog::SyncEditToSlider(HWND hwndEdit, HWND hwndSlider)
{
	wchar_t text[16];
	GetWindowText(hwndEdit, text, _countof(text));
	try {
		int val = std::stoi(text);
		if (val >= 0 && val <= 255) {
			SendMessage(hwndSlider, TBM_SETPOS, TRUE, val);
		}
	} catch (...) {}
}

void DisplayColorsDialog::SaveColors()
{
	auto getColor = [this](int rEdit, int gEdit, int bEdit) -> COLORREF
	{
		int r = GetDlgItemInt(m_hDlg, rEdit, nullptr, FALSE);
		int g = GetDlgItemInt(m_hDlg, gEdit, nullptr, FALSE);
		int b = GetDlgItemInt(m_hDlg, bEdit, nullptr, FALSE);
		return RGB(r, g, b);
	};

	m_config->displayWindowCentreColor = getColor(IDC_EDIT_CENTRE_RED, IDC_EDIT_CENTRE_GREEN, IDC_EDIT_CENTRE_BLUE);
	m_config->displayWindowSurroundColor = getColor(IDC_EDIT_SURROUND_RED, IDC_EDIT_SURROUND_GREEN, IDC_EDIT_SURROUND_BLUE);
}

INT_PTR DisplayColorsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
	WORD id = LOWORD(wParam);
	WORD code = HIWORD(wParam);

	switch (id)
	{
	case IDOK:
		SaveColors();
		EndDialog(m_hDlg, 1);
		break;

	case IDCANCEL:
		EndDialog(m_hDlg, 0);
		break;

	case IDC_BUTTON_RESTOREDEFAULTS:
		m_config->displayWindowCentreColor = DisplayWindowDefaults::CENTRE_COLOR;
		m_config->displayWindowSurroundColor = DisplayWindowDefaults::SURROUND_COLOR;
		ApplyColorsToSliders();
		break;

	case IDC_EDIT_CENTRE_RED:
		if (code == EN_CHANGE) SyncEditToSlider(reinterpret_cast<HWND>(lParam), GetDlgItem(m_hDlg, IDC_SLIDER_CENTRE_RED));
		break;
	case IDC_EDIT_CENTRE_GREEN:
		if (code == EN_CHANGE) SyncEditToSlider(reinterpret_cast<HWND>(lParam), GetDlgItem(m_hDlg, IDC_SLIDER_CENTRE_GREEN));
		break;
	case IDC_EDIT_CENTRE_BLUE:
		if (code == EN_CHANGE) SyncEditToSlider(reinterpret_cast<HWND>(lParam), GetDlgItem(m_hDlg, IDC_SLIDER_CENTRE_BLUE));
		break;
	case IDC_EDIT_SURROUND_RED:
		if (code == EN_CHANGE) SyncEditToSlider(reinterpret_cast<HWND>(lParam), GetDlgItem(m_hDlg, IDC_SLIDER_SURROUND_RED));
		break;
	case IDC_EDIT_SURROUND_GREEN:
		if (code == EN_CHANGE) SyncEditToSlider(reinterpret_cast<HWND>(lParam), GetDlgItem(m_hDlg, IDC_SLIDER_SURROUND_GREEN));
		break;
	case IDC_EDIT_SURROUND_BLUE:
		if (code == EN_CHANGE) SyncEditToSlider(reinterpret_cast<HWND>(lParam), GetDlgItem(m_hDlg, IDC_SLIDER_SURROUND_BLUE));
		break;
	}

	return 0;
}

INT_PTR DisplayColorsDialog::OnHScroll(HWND hwnd)
{
	int id = GetDlgCtrlID(hwnd);
	switch (id)
	{
	case IDC_SLIDER_CENTRE_RED:
		SyncSliderToEdit(hwnd, GetDlgItem(m_hDlg, IDC_EDIT_CENTRE_RED));
		break;
	case IDC_SLIDER_CENTRE_GREEN:
		SyncSliderToEdit(hwnd, GetDlgItem(m_hDlg, IDC_EDIT_CENTRE_GREEN));
		break;
	case IDC_SLIDER_CENTRE_BLUE:
		SyncSliderToEdit(hwnd, GetDlgItem(m_hDlg, IDC_EDIT_CENTRE_BLUE));
		break;
	case IDC_SLIDER_SURROUND_RED:
		SyncSliderToEdit(hwnd, GetDlgItem(m_hDlg, IDC_EDIT_SURROUND_RED));
		break;
	case IDC_SLIDER_SURROUND_GREEN:
		SyncSliderToEdit(hwnd, GetDlgItem(m_hDlg, IDC_EDIT_SURROUND_GREEN));
		break;
	case IDC_SLIDER_SURROUND_BLUE:
		SyncSliderToEdit(hwnd, GetDlgItem(m_hDlg, IDC_EDIT_SURROUND_BLUE));
		break;
	}
	return 0;
}

INT_PTR DisplayColorsDialog::OnClose()
{
	EndDialog(m_hDlg, 0);
	return 0;
}
