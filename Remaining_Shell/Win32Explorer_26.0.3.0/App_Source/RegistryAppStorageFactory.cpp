// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "RegistryAppStorageFactory.h"
#include "RegistryAppStorage.h"
#include <wil/registry.h>

std::unique_ptr<RegistryAppStorage> RegistryAppStorageFactory::MaybeCreate(
	const std::wstring &applicationKeyPath, Storage::OperationType operationType, bool useHKLM)
{
	wil::unique_hkey applicationKey;
	HKEY rootKey = useHKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;

	if (operationType == Storage::OperationType::Load)
	{
		applicationKey = OpenKeyForLoad(applicationKeyPath, useHKLM);
	}
	else
	{
		// Settings are going to be saved, so remove the existing application key first. It's
		// important to do this to ensure that, when saving a list of items (e.g. a list of tabs or
		// bookmarks), the existing items are removed before the updated list is stored. Otherwise,
		// if the list shrinks, some of the previous items won't be removed.
		SHDeleteKey(rootKey, applicationKeyPath.c_str());

		applicationKey = CreateKeyForSave(applicationKeyPath, useHKLM);
	}

	if (!applicationKey)
	{
		return nullptr;
	}

	return std::make_unique<RegistryAppStorage>(std::move(applicationKey));
}

wil::unique_hkey RegistryAppStorageFactory::OpenKeyForLoad(const std::wstring &applicationKeyPath, bool useHKLM)
{
	wil::unique_hkey applicationKey;
	HKEY rootKey = useHKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	HRESULT hr = wil::reg::open_unique_key_nothrow(rootKey, applicationKeyPath.c_str(),
		applicationKey, wil::reg::key_access::read);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return applicationKey;
}

wil::unique_hkey RegistryAppStorageFactory::CreateKeyForSave(const std::wstring &applicationKeyPath, bool useHKLM)
{
	wil::unique_hkey applicationKey;
	HKEY rootKey = useHKLM ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
	HRESULT hr = wil::reg::create_unique_key_nothrow(rootKey, applicationKeyPath.c_str(),
		applicationKey, wil::reg::key_access::readwrite);

	if (FAILED(hr))
	{
		return nullptr;
	}

	return applicationKey;
}

