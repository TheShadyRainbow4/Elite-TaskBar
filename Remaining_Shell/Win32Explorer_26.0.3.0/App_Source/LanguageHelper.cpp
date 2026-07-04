// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "LanguageHelper.h"
#include "CommandLine.h"
#include "Config.h"
#include "../Shared_Libraries/Helper.h"
#include "../Shared_Libraries/ProcessHelper.h"

namespace LanguageHelper
{

std::variant<LanguageInfo, LoadError> MaybeLoadTranslationDll(
	const CommandLine::Settings * /*commandLineSettings*/, const Config * /*config*/)
{
	/* All translations have been removed in favor of a single English version. */
	return LoadError::LanguageCodeNotFound;
}

bool IsLanguageRTL(LANGID /*language*/)
{
	/* English is not an RTL language. */
	return false;
}

}

