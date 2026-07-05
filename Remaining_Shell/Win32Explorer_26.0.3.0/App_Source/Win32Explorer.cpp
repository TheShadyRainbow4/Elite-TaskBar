// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "Win32Explorer.h"
#include "App.h"
#include "Application.h"
#include "Bookmarks/BookmarkIconManager.h"
#include "Bookmarks/UI/BookmarksMainMenu.h"
#include "Config.h"
#include "DisplayWindow/DisplayWindow.h"
#include "FrequentLocationsMenu.h"
#include "HistoryMenu.h"
#include "HolderWindow.h"
#include "MainFontSetter.h"
#include "MainMenuSubMenuView.h"
#include "MainRebarStorage.h"
#include "MainRebarView.h"
#include "MainResource.h"
#include "MainToolbar.h"
#include "MainWindow.h"
#include "MenuRanges.h"
#include "Plugins/PluginManager.h"
#include "ResourceLoader.h"
#include "ShellBrowser/ShellBrowserImpl.h"
#include "ShellTreeView/ShellTreeView.h"
#include "StatusBar.h"
#include "StatusBarView.h"
#include "TabRestorer.h"
#include "TabRestorerMenu.h"
#include "TabStorage.h"
#include "TaskbarThumbnails.h"
#include "ThemeWindowTracker.h"
#include "WindowStorage.h"
#include "../Shared_Libraries/WindowHelper.h"
#include "../Shared_Libraries/WindowSubclass.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

Win32Explorer *Win32Explorer::Create(App *app, const WindowStorageData *storageData)
{
	return new Win32Explorer(app, storageData);
}

Win32Explorer::Win32Explorer(App *app, const WindowStorageData *storageData) :
	m_app(app),
	m_hContainer(CreateMainWindow(storageData)),
	m_commandController(this, app->GetConfig(), app->GetPlatformContext()->GetClipboardStore(),
		app->GetResourceLoader()),
	m_tabBarBackgroundBrush(CreateSolidBrush(TAB_BAR_DARK_MODE_BACKGROUND_COLOR)),
	m_pluginMenuManager(m_hContainer, MENU_PLUGIN_START_ID, MENU_PLUGIN_END_ID),
	m_acceleratorUpdater(app->GetAcceleratorManager()),
	m_pluginCommandManager(app->GetAcceleratorManager(), ACCELERATOR_PLUGIN_START_ID,
		ACCELERATOR_PLUGIN_END_ID),
	m_shellBrowserFactory(app, this, &m_fileActionHandler),
	m_config(app->GetConfig()),
	m_iconFetcher(m_hContainer, m_app->GetCachedIcons()),
	m_shellIconLoader(&m_iconFetcher),
	m_applicationExecutor(this)
{
	m_bShowTabBar = true;
	m_pActiveShellBrowser = nullptr;
	m_lastActiveWindow = nullptr;
	m_hActiveListView = nullptr;

	m_iDWFolderSizeUniqueId = 0;

	if (storageData)
	{
		m_treeViewWidth = storageData->treeViewWidth;
		m_displayWindowWidth = storageData->displayWindowWidth;
		m_displayWindowHeight = storageData->displayWindowHeight;
	}

	SetUpControlVisibilityConfigListeners();

	m_windowSubclasses.push_back(std::make_unique<WindowSubclass>(m_hContainer,
		std::bind_front(&Win32Explorer::WindowProcedure, this)));

	Initialize(storageData);

	WindowShowState showState = storageData ? storageData->showState : +WindowShowState::Normal;

	if (showState == +WindowShowState::Minimized)
	{
		showState = WindowShowState::Normal;
	}

	ShowWindow(m_hContainer, ShowStateToNativeShowState(showState));
	UpdateWindow(m_hContainer);

	m_app->GetBrowserList()->AddBrowser(this);
}

Win32Explorer::~Win32Explorer() = default;

HWND Win32Explorer::CreateMainWindow(const WindowStorageData *storageData)
{
	static bool mainWindowClassRegistered = false;

	if (!mainWindowClassRegistered)
	{
		auto res = RegisterMainWindowClass(GetModuleHandle(nullptr));
		CHECK(res);

		mainWindowClassRegistered = true;
	}

	HWND hwnd = CreateWindow(WINDOW_CLASS_NAME, App::APP_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, GetModuleHandle(nullptr),
		nullptr);
	CHECK(hwnd);

	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(hwnd, &placement);
	CHECK(res);

	placement.showCmd = SW_HIDE;
	placement.rcNormalPosition =
		storageData ? storageData->bounds : LayoutDefaults::GetDefaultMainWindowBounds();
	SetWindowPlacement(hwnd, &placement);

	return hwnd;
}

ATOM Win32Explorer::RegisterMainWindowClass(HINSTANCE instance)
{
	WNDCLASSEX windowClass = {};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.style = 0;
	windowClass.lpfnWndProc = DefWindowProc;
	windowClass.cbClsExtra = 0;
	windowClass.cbWndExtra = 0;
	windowClass.hInstance = instance;
	windowClass.hIcon = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN),
		IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
	windowClass.hIconSm = static_cast<HICON>(LoadImage(instance, MAKEINTRESOURCE(IDI_MAIN),
		IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
	windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
	windowClass.hbrBackground = nullptr;
	windowClass.lpszMenuName = nullptr;
	windowClass.lpszClassName = WINDOW_CLASS_NAME;
	return RegisterClassEx(&windowClass);
}

void Win32Explorer::SetUpControlVisibilityConfigListeners()
{
	m_connections.push_back(
		m_config->showStatusBar.addObserver(std::bind(&Win32Explorer::UpdateLayout, this)));
	m_connections.push_back(
		m_config->showFolders.addObserver(std::bind(&Win32Explorer::UpdateLayout, this)));
	m_connections.push_back(
		m_config->showDisplayWindow.addObserver(std::bind(&Win32Explorer::UpdateLayout, this)));

	m_connections.push_back(
		m_config->configChangedSignal.connect(std::bind(&Win32Explorer::OnConfigChanged, this)));
}

void Win32Explorer::OnConfigChanged()
{
	UpdateLayout();

	// Additional refresh logic for components that don't use observers.
	MaybeUpdateTabBarVisibility();
	
	// Refresh the active shell browser view mode, grouping, etc.
	// This ensures that changes like "Show in groups" or "View mode" apply instantly.
	auto *shellBrowser = GetActiveShellBrowserImpl();
	if (shellBrowser)
	{
		shellBrowser->UpdateFolderSettings(m_config->defaultFolderSettings);
	}

	InvalidateRect(m_hContainer, nullptr, TRUE);
}

void Win32Explorer::Initialize(const WindowStorageData *storageData)
{
	m_bookmarksMainMenu = std::make_unique<BookmarksMainMenu>(m_app, this, this,
		m_app->GetResourceLoader(), &m_iconFetcher, m_app->GetBookmarkTree(),
		BookmarkMenuBuilder::MenuIdRange{ MENU_BOOKMARK_START_ID, MENU_BOOKMARK_END_ID });

	m_mainWindow = MainWindow::Create(m_hContainer, m_app, this);

	InitializeMainMenu();

	auto *statusBarView = StatusBarView::Create(m_hContainer, m_config);
	m_statusBar = StatusBar::Create(statusBarView, this, m_config, m_app->GetTabEvents(),
		m_app->GetShellBrowserEvents(), m_app->GetNavigationEvents(), m_app->GetResourceLoader());

	CreateMainRebarAndChildren(storageData);
	InitializeDisplayWindow();
	InitializeTabs();
	CreateFolderControls();

	/* All child windows MUST be resized before
	any listview changes take place. If auto arrange
	is turned off in the listview, when it is
	initially sized, all current items will lock
	to the current width. The only was to unlock
	them from this width is to turn auto arrange back on.
	Therefore, the listview MUST be set to the correct
	size initially. */
	UpdateLayout();

	m_taskbarThumbnails =
		std::make_unique<TaskbarThumbnails>(m_app, this, GetActivePane()->GetTabContainer());

	CreateInitialTabs(storageData);

	SetFocus(m_hActiveListView);

	InitializePlugins();

	m_themeWindowTracker =
		std::make_unique<ThemeWindowTracker>(m_hContainer, m_app->GetThemeManager());

	SetLifecycleState(LifecycleState::Main);
}

void Win32Explorer::InitializeDisplayWindow()
{
	m_displayWindow = DisplayWindow::Create(m_hContainer, m_config);

	ApplyDisplayWindowPosition();
}

void Win32Explorer::CreateFolderControls()
{
	UINT holderStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	if (m_config->showFolders.get())
	{
		holderStyle |= WS_VISIBLE;
	}

	m_treeViewHolder = HolderWindow::Create(m_hContainer,
		m_app->GetResourceLoader()->LoadString(IDS_FOLDERS_WINDOW_TEXT), holderStyle,
		m_app->GetResourceLoader()->LoadString(IDS_HIDE_FOLDERS_PANE), m_app->GetConfig(),
		m_app->GetResourceLoader(), m_app->GetDarkModeManager(), m_app->GetDarkModeColorProvider());
	m_treeViewHolder->SetCloseButtonClickedCallback(
		[this]() { m_config->showFolders = !m_config->showFolders.get(); });
	m_treeViewHolder->SetResizedCallback(
		std::bind_front(&Win32Explorer::OnTreeViewHolderResized, this));

	m_shellTreeView =
		ShellTreeView::Create(m_treeViewHolder->GetHWND(), m_app, this, &m_fileActionHandler);
	m_treeViewHolder->SetContentChild(m_shellTreeView->GetHWND());
}

void Win32Explorer::OnTreeViewHolderResized(int newWidth)
{
	m_treeViewWidth = newWidth;

	UpdateLayout();
}

Tab *Win32Explorer::CreateTabFromPreservedTab(const PreservedTab *tab)
{
	return &GetActivePane()->GetTabContainer()->CreateNewTab(*tab);
}

HWND Win32Explorer::GetHWND() const
{
	return m_hContainer;
}

WindowStorageData Win32Explorer::GetStorageData() const
{
	WINDOWPLACEMENT placement = {};
	placement.length = sizeof(placement);
	BOOL res = GetWindowPlacement(m_hContainer, &placement);
	CHECK(res);

	const auto *tabContainer = GetActivePane()->GetTabContainer();

	return { .bounds = placement.rcNormalPosition,
		.showState = NativeShowStateToShowState(placement.showCmd),
		.tabs = tabContainer->GetStorageData(),
		.selectedTab = tabContainer->GetSelectedTabIndex(),
		.mainRebarInfo = m_mainRebarView->GetStorageData(),
		.mainToolbarButtons = m_mainToolbar->GetButtonsForStorage(),
		.treeViewWidth = m_treeViewWidth,
		.displayWindowWidth = m_displayWindowWidth,
		.displayWindowHeight = m_displayWindowHeight };
}

bool Win32Explorer::IsActive() const
{
	return GetActiveWindow() == m_hContainer;
}

void Win32Explorer::Activate()
{
	BringWindowToForeground(m_hContainer);
}

void Win32Explorer::TryClose()
{
	if (!ConfirmClose())
	{
		return;
	}

	Close();
}

bool Win32Explorer::ConfirmClose()
{
	if (!m_config->confirmCloseTabs)
	{
		return true;
	}

	auto numTabs = GetActivePane()->GetTabContainer()->GetNumTabs();

	if (numTabs == 1)
	{
		return true;
	}

	std::wstring message =
		fmt::format(fmt::runtime(m_app->GetResourceLoader()->LoadString(IDS_CLOSE_ALL_TABS)),
			fmt::arg(L"num_tabs", numTabs));
	int response =
		MessageBox(m_hContainer, message.c_str(), App::APP_NAME, MB_ICONINFORMATION | MB_YESNO);

	if (response == IDNO)
	{
		return false;
	}

	return true;
}

void Win32Explorer::Close()
{
	if (GetLifecycleState() != LifecycleState::Main)
	{
		return;
	}

	BeginShutdown();

	// When the last tab is closed, the window will be destroyed.
	GetActiveTabContainer()->CloseAllTabs();
}

void Win32Explorer::BeginShutdown()
{
	if (GetLifecycleState() != LifecycleState::Main)
	{
		return;
	}

	SetLifecycleState(LifecycleState::WillClose);

	m_app->GetBrowserList()->WillRemoveBrowser(this);

	// Past this point, the remaining tabs will be closed, which can cause other UI updates. There's
	// no need for that to be shown, however, since the window will be destroyed shortly afterwards.
	ShowWindow(m_hContainer, SW_HIDE);
}

void Win32Explorer::FinishShutdown()
{
	SetLifecycleState(LifecycleState::Closing);

	m_app->GetBrowserList()->RemoveBrowser(this);

	DestroyWindow(m_hContainer);
}

BrowserCommandController *Win32Explorer::GetCommandController()
{
	return &m_commandController;
}

BrowserPane *Win32Explorer::GetActivePane() const
{
	return m_browserPane.get();
}

TabContainer *Win32Explorer::GetActiveTabContainer()
{
	return GetActivePane()->GetTabContainer();
}

const TabContainer *Win32Explorer::GetActiveTabContainer() const
{
	return GetActivePane()->GetTabContainer();
}

ShellBrowser *Win32Explorer::GetActiveShellBrowser()
{
	return GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}

const ShellBrowser *Win32Explorer::GetActiveShellBrowser() const
{
	return GetActivePane()->GetTabContainer()->GetSelectedTab().GetShellBrowser();
}

void Win32Explorer::StartMainToolbarCustomization()
{
	m_mainToolbar->StartCustomization();
}

void Win32Explorer::MenuItemSelected(HMENU menu, UINT itemId, UINT flags)
{
	m_statusBar->OnMenuSelect(menu, itemId, flags);
}

boost::signals2::connection Win32Explorer::AddMenuHelpTextRequestObserver(
	const MenuHelpTextRequestSignal::slot_type &observer)
{
	return m_menuHelpTextRequestSignal.connect(observer);
}

std::optional<std::wstring> Win32Explorer::RequestMenuHelpText(HMENU menu, UINT id) const
{
	auto helpText = m_menuHelpTextRequestSignal(menu, id);

	if (!helpText)
	{
		helpText = m_app->GetResourceLoader()->MaybeLoadString(id);
	}

	return helpText;
}


