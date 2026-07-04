// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Shared_Libraries/BetterEnumsWrapper.h"
#include <string>

class ResourceLoader;

// clang-format off
BETTER_ENUM(Theme, int,
	Light = 0,
	System = 2
)
// clang-format on

std::wstring GetThemeText(Theme theme, const ResourceLoader *resourceLoader);


