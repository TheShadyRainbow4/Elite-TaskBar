// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "../Shared_Libraries/Pidl.h"
#include <memory>

struct FolderColumns;
struct FolderSettings;
struct PreservedShellBrowser;
class ShellBrowser;

class ShellBrowserFactory
{
public:
	virtual ~ShellBrowserFactory() = default;

	virtual std::unique_ptr<ShellBrowser> Create(const PidlAbsolute &initialPidl,
		const FolderSettings &folderSettings, const FolderColumns *initialColumns) = 0;
	virtual std::unique_ptr<ShellBrowser> CreateFromPreserved(
		const PreservedShellBrowser &preservedShellBrowser) = 0;
};


