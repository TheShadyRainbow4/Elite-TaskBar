// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Shared_Libraries/PidlHelper.h"
#include "../Shared_Libraries/ShellHelper.h"
#include "../Shared_Libraries/WinRTBaseWrapper.h"
#include <wil/com.h>

void CreateTextDataObject(const std::wstring &text, winrt::com_ptr<IDataObject> &dataObject);
void CreateShellDataObject(const std::wstring &path, ShellItemType shellItemType,
	wil::com_ptr_nothrow<IDataObject> &dataObject);


