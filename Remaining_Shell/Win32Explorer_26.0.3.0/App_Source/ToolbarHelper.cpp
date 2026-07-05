// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "ToolbarHelper.h"
#include "Icon.h"
#include "ResourceLoader.h"
#include "../Shared_Libraries/Controls.h"
#include "../Shared_Libraries/DpiCompatibility.h"

namespace ToolbarHelper
{

std::tuple<HWND, std::vector<wil::unique_himagelist>> CreateCloseButtonToolbar(HWND parent,
	int closeButtonId, const std::wstring &tooltip, const ResourceLoader *resourceLoader, int iconSize, bool isTab)
{
	DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | TBSTYLE_TOOLTIPS
			| TBSTYLE_TRANSPARENT | TBSTYLE_FLAT | CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | TBSTYLE_LIST;

	HWND toolbar = CreateToolbar(parent, style, TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER);
	SendMessage(toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	UINT dpi = DpiCompatibility::GetInstance().GetDpiForWindow(toolbar);

	wil::unique_hbitmap normalBitmap =
		resourceLoader->LoadBitmapFromPNGForDpi(Icon::CloseButton, iconSize, iconSize, dpi);
	wil::unique_hbitmap pressedBitmap =
		resourceLoader->LoadBitmapFromPNGForDpi(Icon::PressedCloseButton, iconSize, iconSize, dpi);

	BITMAP bitmapInfo;
	[[maybe_unused]] int res = GetObject(normalBitmap.get(), sizeof(bitmapInfo), &bitmapInfo);
	assert(res != 0);

	std::vector<wil::unique_himagelist> imageLists;

	wil::unique_himagelist imageList(
		ImageList_Create(bitmapInfo.bmWidth, bitmapInfo.bmHeight, ILC_COLOR32 | ILC_MASK, 0, 1));
	int normalImageIndex = ImageList_Add(imageList.get(), normalBitmap.get(), nullptr);

	wil::unique_himagelist hotImageList(
		ImageList_Create(bitmapInfo.bmWidth, bitmapInfo.bmHeight, ILC_COLOR32 | ILC_MASK, 0, 1));
	ImageList_Add(hotImageList.get(), pressedBitmap.get(), nullptr);

	if (isTab) {
		SendMessage(toolbar, TB_SETPADDING, 0, MAKELPARAM(0, 0));
	}
	SendMessage(toolbar, TB_SETBITMAPSIZE, 0, MAKELONG(bitmapInfo.bmWidth, bitmapInfo.bmHeight));
	SendMessage(toolbar, TB_SETBUTTONSIZE, 0, MAKELPARAM(bitmapInfo.bmWidth, bitmapInfo.bmHeight));

	SendMessage(toolbar, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(imageList.get()));
	SendMessage(toolbar, TB_SETHOTIMAGELIST, 0, reinterpret_cast<LPARAM>(hotImageList.get()));
	SendMessage(toolbar, TB_SETPRESSEDIMAGELIST, 0, reinterpret_cast<LPARAM>(hotImageList.get()));

	TBBUTTON tbButton = {};
	tbButton.iBitmap = normalImageIndex;
	tbButton.idCommand = closeButtonId;
	tbButton.fsState = TBSTATE_ENABLED;
	tbButton.fsStyle = TBSTYLE_BUTTON;
	if (!isTab) {
		tbButton.fsStyle |= TBSTYLE_AUTOSIZE;
	}
	tbButton.dwData = 0;
	tbButton.iString = reinterpret_cast<INT_PTR>(tooltip.c_str());
	SendMessage(toolbar, TB_INSERTBUTTON, 0, reinterpret_cast<LPARAM>(&tbButton));

	SendMessage(toolbar, TB_AUTOSIZE, 0, 0);

	imageLists.push_back(std::move(imageList));
	imageLists.push_back(std::move(hotImageList));

	return { toolbar, std::move(imageLists) };
}

}


