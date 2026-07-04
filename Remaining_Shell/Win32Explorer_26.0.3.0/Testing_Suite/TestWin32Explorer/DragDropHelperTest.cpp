// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "pch.h"
#include "../Shared_Libraries/DragDropHelper.h"
#include "DragDropTestHelper.h"
#include "../Shared_Libraries/DataObjectImpl.h"
#include "../Shared_Libraries/WinRTBaseWrapper.h"
#include <gtest/gtest.h>

using namespace testing;

class PreferredDropEffectTestSuite : public TestWithParam<DWORD>
{
};

TEST_P(PreferredDropEffectTestSuite, PreferredDropEffect)
{
	wil::com_ptr_nothrow<IDataObject> dataObject;
	CreateShellDataObject(L"C:\\fake", ShellItemType::File, dataObject);

	DWORD assignedEffect = GetParam();
	EXPECT_HRESULT_SUCCEEDED(SetPreferredDropEffect(dataObject.get(), assignedEffect));

	DWORD retrievedEffect = DROPEFFECT_NONE;
	EXPECT_HRESULT_SUCCEEDED(GetPreferredDropEffect(dataObject.get(), retrievedEffect));

	EXPECT_EQ(retrievedEffect, assignedEffect);
}

INSTANTIATE_TEST_SUITE_P(CopyAndMoveEffects, PreferredDropEffectTestSuite,
	Values(DROPEFFECT_COPY, DROPEFFECT_MOVE));

TEST(DragDropHelperTest, GetSetTextOnDataObject)
{
	auto dataObject = winrt::make<DataObjectImpl>();

	std::wstring text = L"Test text";
	ASSERT_HRESULT_SUCCEEDED(SetTextOnDataObject(dataObject.get(), text));

	std::wstring retrievedText;
	ASSERT_HRESULT_SUCCEEDED(GetTextFromDataObject(dataObject.get(), retrievedText));
	EXPECT_EQ(retrievedText, text);
}


