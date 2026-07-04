// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#pragma once

#include "PlatformContext.h"
#include "../Shared_Libraries/KeyboardStateImpl.h"
#include "../Shared_Libraries/SystemClipboardStore.h"
#include "../Shared_Libraries/SystemClockImpl.h"

class PlatformContextImpl : public PlatformContext
{
public:
	SystemClock *GetSystemClock() override;
	KeyboardState *GetKeyboardState() override;
	ClipboardStore *GetClipboardStore() override;

private:
	SystemClockImpl m_systemClock;
	KeyboardStateImpl m_keyboardState;
	SystemClipboardStore m_clipboardStore;
};


