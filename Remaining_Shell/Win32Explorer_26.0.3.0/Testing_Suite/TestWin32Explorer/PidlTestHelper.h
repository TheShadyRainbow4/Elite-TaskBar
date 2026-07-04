// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Shared_Libraries/PidlHelper.h"

class PidlAbsolute;

PidlAbsolute CreateSimplePidlForTest(const std::wstring &path, IShellFolder *parent = nullptr,
	ShellItemType shellItemType = ShellItemType::Folder,
	ShellItemExtraAttributes extraAttributes = ShellItemExtraAttributes::None);


