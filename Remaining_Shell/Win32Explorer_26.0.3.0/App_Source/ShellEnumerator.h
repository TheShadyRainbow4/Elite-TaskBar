// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "ShellItemFilter.h"
#include "../Shared_Libraries/Pidl.h"
#include <stop_token>
#include <vector>

class ShellEnumerator
{
public:
	virtual ~ShellEnumerator() = default;

	virtual HRESULT EnumerateDirectory(PCIDLIST_ABSOLUTE pidlDirectory,
		ShellItemFilter::ItemType itemType, ShellItemFilter::HiddenItemPolicy hiddenItemPolicy,
		std::vector<PidlChild> &outputItems, std::stop_token stopToken) const = 0;
};


