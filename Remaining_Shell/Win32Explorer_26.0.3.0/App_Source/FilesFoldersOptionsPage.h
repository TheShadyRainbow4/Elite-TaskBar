// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "OptionsPage.h"
#include "../Shared_Libraries/StringHelper.h"

class CoreInterface;

class FilesFoldersOptionsPage : public OptionsPage
{
public:
	FilesFoldersOptionsPage(HWND parent, const ResourceLoader *resourceLoader, Config *config,
		SettingChangedCallback settingChangedCallback, HWND tooltipWindow,
		CoreInterface *coreInterface);

	void SaveSettings() override;

private:
	std::unique_ptr<ResizableDialogHelper> InitializeResizeDialogHelper() override;
	void InitializeControls() override;

	void OnCommand(WPARAM wParam, LPARAM lParam) override;

	void SetInfoTipControlStates();
	void SetFolderSizeControlState();
	std::wstring GetSizeDisplayFormatText(SizeDisplayFormat sizeDisplayFormat);

	CoreInterface *const m_coreInterface;
};


