// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include <wil/resource.h>
#include <string>
#include <tuple>

#include <vector>

class ResourceLoader;

namespace ToolbarHelper
{

static constexpr int CLOSE_TOOLBAR_X_OFFSET = 4;

std::tuple<HWND, std::vector<wil::unique_himagelist>> CreateCloseButtonToolbar(HWND parent,
	int buttonId, const std::wstring &tooltipText, const ResourceLoader *resourceLoader, int iconSize = 24, bool isTab = false);

}
