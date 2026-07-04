// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "NavigationHelper.h"
#include "../Shared_Libraries/Helper.h"

OpenFolderDisposition DetermineOpenDisposition(bool isMiddleButtonDown)
{
	return DetermineOpenDisposition(isMiddleButtonDown, IsKeyDown(VK_CONTROL), IsKeyDown(VK_SHIFT));
}

OpenFolderDisposition DetermineOpenDisposition(bool isMiddleButtonDown, bool isCtrlKeyDown,
	bool isShiftKeyDown)
{
	if (isMiddleButtonDown || isCtrlKeyDown)
	{
		if (!isShiftKeyDown)
		{
			return OpenFolderDisposition::NewTabDefault;
		}
		else
		{
			// Shift inverts the usual behavior.
			return OpenFolderDisposition::NewTabAlternate;
		}
	}

	if (isShiftKeyDown)
	{
		return OpenFolderDisposition::NewWindow;
	}

	return OpenFolderDisposition::CurrentTab;
}


