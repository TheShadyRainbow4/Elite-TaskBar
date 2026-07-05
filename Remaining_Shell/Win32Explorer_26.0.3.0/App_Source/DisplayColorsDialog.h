// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "BaseDialog.h"
#include <wil/resource.h>

struct Config;

class DisplayColorsDialog : public BaseDialog
{
public:
	static DisplayColorsDialog *Create(const ResourceLoader *resourceLoader, HWND hParent, Config *config);
	COLORREF GetCurrentCenterColor();
	COLORREF GetCurrentSurroundColor();

protected:
	INT_PTR OnInitDialog() override;
	INT_PTR OnCommand(WPARAM wParam, LPARAM lParam) override;
	INT_PTR OnHScroll(HWND hwnd) override;
	INT_PTR OnClose() override;

private:
	DisplayColorsDialog(const ResourceLoader *resourceLoader, HWND hParent, Config *config);
	~DisplayColorsDialog() = default;

	void SyncSliderToEdit(HWND hwndSlider, HWND hwndEdit);
	void SyncEditToSlider(HWND hwndEdit, HWND hwndSlider);
	void ApplyColorsToSliders();
	void SaveColors();

	void InvalidatePreview();

	Config *m_config;
};
