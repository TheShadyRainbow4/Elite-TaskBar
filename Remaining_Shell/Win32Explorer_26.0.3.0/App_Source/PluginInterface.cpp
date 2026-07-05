// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Win32Explorer.h"

TabContainer *Win32Explorer::GetTabContainer()
{
	return GetActivePane()->GetTabContainer();
}

Plugins::PluginMenuManager *Win32Explorer::GetPluginMenuManager()
{
	return &m_pluginMenuManager;
}

AcceleratorUpdater *Win32Explorer::GetAccleratorUpdater()
{
	return &m_acceleratorUpdater;
}

Plugins::PluginCommandManager *Win32Explorer::GetPluginCommandManager()
{
	return &m_pluginCommandManager;
}

