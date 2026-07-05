// Copyright (C) Win32Explorer Project
// SPDX-License-Identifier: GPL-3.0-only
// See LICENSE in the top level directory

#include "stdafx.h"
#include "App.h"
#include "EliteTaskbar/resource.h"
#define WM_TRAYICON (WM_USER + 200)
#include "Win32Explorer.h"
#include "AsyncIconFetcher.h"
#include "BrowserWindow.h"
#include "ColumnStorage.h"
#include "ComStaThreadPoolExecutor.h"
#include "DefaultAccelerators.h"
#include "DriveEnumeratorImpl.h"
#include "ExitCode.h"
#include "FileSystemWatcher.h"
#include "LanguageHelper.h"
#include "MainRebarStorage.h"
#include "MainResource.h"
#include "RegistryAppStorage.h"
#include "RegistryAppStorageFactory.h"
#include "ResourceHelper.h"
#include "ShellWatcher.h"
#include "TabStorage.h"
#include "UIThreadExecutor.h"
#include "Win32ResourceLoader.h"
#include "WindowStorage.h"
#include "XmlAppStorage.h"
#include "XmlAppStorageFactory.h"
#include "../Shared_Libraries/CachedIcons.h"
#include "../Shared_Libraries/Helper.h"
#include "EliteTaskbar/TaskbarMain.h"
#include <fmt/format.h>
#include <fmt/xchar.h>

using namespace std::chrono_literals;

App::App(const CommandLine::Settings *commandLineSettings) :
	m_commandLineSettings(commandLineSettings),
	m_runtime(std::make_unique<UIThreadExecutor>(),
		std::make_unique<ComStaThreadPoolExecutor>(std::max(
			static_cast<int>(std::thread::hardware_concurrency()), MIN_COM_STA_THREADPOOL_SIZE))),
	m_featureList(commandLineSettings->featuresToEnable),
	m_acceleratorManager(InitializeAcceleratorManager()),
	m_directoryWatcherFactory(&m_config, &m_shellWatcherManager, m_runtime.GetUiThreadExecutor()),
	m_darkModeManager(&m_eventWindow, &m_config),
	m_themeManager(&m_darkModeManager, &m_darkModeColorProvider),
	m_cachedIcons(std::make_shared<CachedIcons>(MAX_CACHED_ICONS)),
	m_iconFetcher(std::make_shared<AsyncIconFetcher>(&m_runtime, m_cachedIcons)),
	m_resourceInstance(GetModuleHandle(nullptr)),
	m_processManager(&m_browserList),
	m_tabList(&m_tabEvents),
	m_tabRestorer(&m_tabEvents, &m_browserList),
	m_historyTracker(&m_historyModel, &m_navigationEvents),
	m_frequentLocationsModel(m_platformContext.GetSystemClock()),
	m_frequentLocationsTracker(&m_frequentLocationsModel, &m_navigationEvents),
	m_driveWatcher(&m_eventWindow),
	m_driveModel(std::make_unique<DriveEnumeratorImpl>(), &m_driveWatcher),
	m_uniqueGdiplusShutdown(CheckedGdiplusStartup()),
	m_richEditLib(LoadSystemLibrary(
		L"Msftedit.dll")), // This is needed for version 5 of the Rich Edit control.
	m_oleCleanup(wil::OleInitialize_failfast())
{
	CHECK(m_richEditLib);

	INITCOMMONCONTROLSEX commonControls = {};
	commonControls.dwSize = sizeof(commonControls);
	commonControls.dwICC = ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_LISTVIEW_CLASSES
		| ICC_USEREX_CLASSES | ICC_STANDARD_CLASSES | ICC_LINK_CLASS;
	BOOL res = InitCommonControlsEx(&commonControls);
	CHECK(res);

	m_browserList.willRemoveBrowserSignal.AddObserver(std::bind(&App::OnWillRemoveBrowser, this));
	m_browserList.browserRemovedSignal.AddObserver(std::bind(&App::OnBrowserRemoved, this));

	if (m_commandLineSettings->changeNotifyMode)
	{
		m_config.changeNotifyMode = *m_commandLineSettings->changeNotifyMode;
	}
	else if (IsWindowsPE())
	{
		m_config.changeNotifyMode = ChangeNotifyMode::Filesystem;
	}

	m_eventWindow.windowMessageSignal.AddObserver(std::bind(&App::OnEventWindowMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

App::~App()
{
	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = m_eventWindow.GetHWND();
	nid.uID = 2;
	Shell_NotifyIconW(NIM_DELETE, &nid);
}

void App::OnBrowserRemoved()
{
	if (m_browserList.IsEmpty() && !IsEliteTaskbarRunning())
	{
		// The last top-level browser window has been closed, so exit the application.
		PostQuitMessage(EXIT_CODE_NORMAL);
	}
}

int App::Run()
{
	SetUpSession();

	// Internally, concurrencpp converts the duration to size_t, which triggers a warning in the
	// 32-bit build. The conversion is fine, since the duration here is well below the point at
	// which truncation would occur and it's not reasonable for the duration to ever be large enough
	// for truncation to occur.
#pragma warning(push)
#pragma warning(                                                                                   \
	disable : 4244) // 'argument': conversion from '_Rep' to 'size_t', possible loss of data
	const auto saveFrequency = 30s;
	m_saveSettingsTimer = m_runtime.GetTimerQueue()->make_timer(saveFrequency, saveFrequency,
		m_runtime.GetUiThreadExecutor(), std::bind_front(&App::SaveSettings, this));
#pragma warning(pop)

	MSG msg;

	while (GetMessage(&msg, nullptr, 0, 0) > 0)
	{
		if (!IsModelessDialogMessage(&msg) && !MaybeTranslateAccelerator(&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return static_cast<int>(msg.wParam);
}

void App::SetUpSession()
{
	std::vector<WindowStorageData> windows;
	LoadSettings(windows);

	// This function may attempt to notify an existing process if the allowMultipleInstances config
	// value is disabled. Therefore, this call needs to be made after the settings have been loaded.
	// If the allowMultipleInstances setting is removed, this call can be made earlier.
	if (!m_processManager.InitializeCurrentProcess(m_commandLineSettings, &m_config))
	{
		PostQuitMessage(EXIT_CODE_NORMAL_EXISTING_PROCESS);
		return;
	}

	SetUpLanguageResourceInstance();

	RestoreSession(windows);

	NOTIFYICONDATAW nid = { 0 };
	nid.cbSize = sizeof(NOTIFYICONDATAW);
	nid.hWnd = m_eventWindow.GetHWND();
	nid.uID = 2;
	nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	nid.uCallbackMessage = WM_TRAYICON;
	nid.hIcon = LoadIconW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(IDI_MAIN_PROGRAM));
	wcscpy_s(nid.szTip, L"Win32Explorer - Because modern WinUI is just too slow for your heavy lifting.");
	Shell_NotifyIconW(NIM_ADD, &nid);
}

void App::LoadSettings(std::vector<WindowStorageData> &windows)
{
	// Settings will be loaded from the config file by default, if that file is present and can be
	// read.
	std::unique_ptr<AppStorage> appStorage = XmlAppStorageFactory::MaybeCreate(
		Storage::GetConfigFilePath(), Storage::OperationType::Load);

	if (appStorage)
	{
		m_savePreferencesToXmlFile = true;
	}
	else
	{
		bool useHKLM = false;
		HKEY hKey = nullptr;
		DWORD dwValue = 0;
		DWORD cbData = sizeof(DWORD);
		if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			if (RegQueryValueExW(hKey, L"EnablePortableMirror", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue == 1)
			{
				useHKLM = true;
			}
			RegCloseKey(hKey);
		}
		if (!useHKLM)
		{
			if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
			{
				if (RegQueryValueExW(hKey, L"EnablePortableMirror", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue == 1)
				{
					useHKLM = true;
				}
				RegCloseKey(hKey);
			}
		}
		appStorage = RegistryAppStorageFactory::MaybeCreate(Storage::REGISTRY_APPLICATION_KEY_PATH,
			Storage::OperationType::Load, useHKLM);
	}

	if (!appStorage)
	{
		return;
	}

	appStorage->LoadConfig(m_config);
	windows = appStorage->LoadWindows();
	appStorage->LoadBookmarks(&m_bookmarkTree);
	appStorage->LoadApplications(&m_applicationModel);
	appStorage->LoadDialogStates();
	appStorage->LoadDefaultColumns(m_config.globalFolderSettings.folderColumns);
	appStorage->LoadFrequentLocations(&m_frequentLocationsModel);

	ValidateColumns(m_config.globalFolderSettings.folderColumns);
}

void App::SaveSettings()
{
	// If the application has started exiting, it's not possible to save the settings, so that's not
	// something that should be attempted. That's because one or more of the windows may have
	// already been closed.
	CHECK(!m_exitStarted);

	std::vector<WindowStorageData> windows;

	for (const auto *browser : m_browserList.GetList())
	{
		windows.push_back(browser->GetStorageData());
	}

	DCHECK_GE(windows.size(), 1u);

	if (m_config.enablePortableMirror.get())
	{
		m_savePreferencesToXmlFile = true; // Make sure it persists in future loads

		// Save simultaneously to XML (config.xml)
		auto xmlStorage = XmlAppStorageFactory::MaybeCreate(Storage::GetConfigFilePath(), Storage::OperationType::Save);
		if (xmlStorage)
		{
			xmlStorage->SaveConfig(m_config);
			xmlStorage->SaveWindows(windows);
			xmlStorage->SaveBookmarks(&m_bookmarkTree);
			xmlStorage->SaveApplications(&m_applicationModel);
			xmlStorage->SaveDialogStates();
			xmlStorage->SaveDefaultColumns(m_config.globalFolderSettings.folderColumns);
			xmlStorage->SaveFrequentLocations(&m_frequentLocationsModel);
			xmlStorage->Commit();
		}
		// Save simultaneously to Registry (HKLM)
		auto regStorage = RegistryAppStorageFactory::MaybeCreate(Storage::REGISTRY_APPLICATION_KEY_PATH, Storage::OperationType::Save, true);
		if (regStorage)
		{
			regStorage->SaveConfig(m_config);
			regStorage->SaveWindows(windows);
			regStorage->SaveBookmarks(&m_bookmarkTree);
			regStorage->SaveApplications(&m_applicationModel);
			regStorage->SaveDialogStates();
			regStorage->SaveDefaultColumns(m_config.globalFolderSettings.folderColumns);
			regStorage->SaveFrequentLocations(&m_frequentLocationsModel);
			regStorage->Commit();
		}
	}
	else
	{
		std::unique_ptr<AppStorage> appStorage;
		if (m_savePreferencesToXmlFile)
		{
			appStorage = XmlAppStorageFactory::MaybeCreate(Storage::GetConfigFilePath(),
				Storage::OperationType::Save);
		}
		else
		{
			appStorage = RegistryAppStorageFactory::MaybeCreate(Storage::REGISTRY_APPLICATION_KEY_PATH,
				Storage::OperationType::Save, false);
		}

		if (appStorage)
		{
			appStorage->SaveConfig(m_config);
			appStorage->SaveWindows(windows);
			appStorage->SaveBookmarks(&m_bookmarkTree);
			appStorage->SaveApplications(&m_applicationModel);
			appStorage->SaveDialogStates();
			appStorage->SaveDefaultColumns(m_config.globalFolderSettings.folderColumns);
			appStorage->SaveFrequentLocations(&m_frequentLocationsModel);
			appStorage->Commit();
		}
	}
}

void App::SetUpLanguageResourceInstance()
{
	/* All translations have been removed in favor of a single English version. */
	m_config.language = LanguageHelper::DEFAULT_LANGUAGE;
	m_resourceInstance = GetModuleHandle(nullptr);

	m_resourceLoader = std::make_unique<Win32ResourceLoader>(m_resourceInstance, m_config.iconSet,
		&m_darkModeManager, &m_themeManager);
}

void App::RestoreSession(const std::vector<WindowStorageData> &windows)
{
	if (m_config.startupMode == +StartupMode::PreviousTabs)
	{
		RestorePreviousWindows(windows);
	}
	else if (m_config.startupMode == +StartupMode::CustomFolders)
	{
		CreateStartupFolders();
	}

	if (m_browserList.IsEmpty())
	{
		// This path can be taken in a few different situations:
		//
		// - If m_config.startupMode is StartupMode::PreviousTabs and the list of windows is empty.
		// - If m_config.startupMode is StartupMode::CustomFolders and the list of startup folders
		//   is empty.
		// - If m_config.startupMode is StartupMode::DefaultFolder.
		//
		// In each case, a default window should be created.
		Win32Explorer::Create(this);
	}
}

void App::RestorePreviousWindows(const std::vector<WindowStorageData> &windows)
{
	for (const auto &window : windows)
	{
		Win32Explorer::Create(this, &window);

		// If this feature isn't enabled, only a single window is supported.
		if (!m_featureList.IsEnabled(Feature::MultipleWindowsPerSession))
		{
			break;
		}
	}
}

void App::CreateStartupFolders()
{
	if (m_config.startupFolders.empty())
	{
		return;
	}

	std::vector<TabStorageData> tabs;

	for (const auto &startupFolder : m_config.startupFolders)
	{
		tabs.push_back({ .directory = startupFolder });
	}

	WindowStorageData initialData;
	initialData.tabs = tabs;
	initialData.selectedTab = 0;
	Win32Explorer::Create(this, &initialData);
}

bool App::IsModelessDialogMessage(MSG *msg)
{
	for (auto modelessDialog : m_modelessDialogList.GetList())
	{
		if (IsChild(modelessDialog, msg->hwnd))
		{
			return IsDialogMessage(modelessDialog, msg);
		}
	}

	return false;
}

bool App::MaybeTranslateAccelerator(MSG *msg)
{
	for (auto *browser : m_browserList.GetList())
	{
		if (IsChild(browser->GetHWND(), msg->hwnd))
		{
			return TranslateAccelerator(browser->GetHWND(),
				m_acceleratorManager.GetAcceleratorTable(), msg);
		}
	}

	return false;
}

const CommandLine::Settings *App::GetCommandLineSettings() const
{
	return m_commandLineSettings;
}

bool App::GetSavePreferencesToXmlFile() const
{
	return m_savePreferencesToXmlFile;
}

void App::SetSavePreferencesToXmlFile(bool savePreferencesToXmlFile)
{
	m_savePreferencesToXmlFile = savePreferencesToXmlFile;
}

PlatformContext *App::GetPlatformContext()
{
	return &m_platformContext;
}

Runtime *App::GetRuntime()
{
	return &m_runtime;
}

ClipboardWatcher *App::GetClipboardWatcher()
{
	return &m_clipboardWatcher;
}

FeatureList *App::GetFeatureList()
{
	return &m_featureList;
}

AcceleratorManager *App::GetAcceleratorManager()
{
	return &m_acceleratorManager;
}

Config *App::GetConfig()
{
	return &m_config;
}

DirectoryWatcherFactory *App::GetDirectoryWatcherFactory()
{
	return &m_directoryWatcherFactory;
}

CachedIcons *App::GetCachedIcons()
{
	return m_cachedIcons.get();
}

std::shared_ptr<AsyncIconFetcher> App::GetIconFetcher()
{
	return m_iconFetcher;
}

BrowserList *App::GetBrowserList()
{
	return &m_browserList;
}

ModelessDialogList *App::GetModelessDialogList()
{
	return &m_modelessDialogList;
}

BookmarkTree *App::GetBookmarkTree()
{
	return &m_bookmarkTree;
}


Applications::ApplicationModel *App::GetApplicationModel()
{
	return &m_applicationModel;
}

HINSTANCE App::GetResourceInstance() const
{
	return m_resourceInstance;
}

ResourceLoader *App::GetResourceLoader() const
{
	return m_resourceLoader.get();
}

TabEvents *App::GetTabEvents()
{
	return &m_tabEvents;
}

ShellBrowserEvents *App::GetShellBrowserEvents()
{
	return &m_shellBrowserEvents;
}

NavigationEvents *App::GetNavigationEvents()
{
	return &m_navigationEvents;
}

TabList *App::GetTabList()
{
	return &m_tabList;
}

TabRestorer *App::GetTabRestorer()
{
	return &m_tabRestorer;
}

DarkModeManager *App::GetDarkModeManager()
{
	return &m_darkModeManager;
}

DarkModeColorProvider *App::GetDarkModeColorProvider()
{
	return &m_darkModeColorProvider;
}

ThemeManager *App::GetThemeManager()
{
	return &m_themeManager;
}

HistoryModel *App::GetHistoryModel()
{
	return &m_historyModel;
}

FrequentLocationsModel *App::GetFrequentLocationsModel()
{
	return &m_frequentLocationsModel;
}

DriveModel *App::GetDriveModel()
{
	return &m_driveModel;
}

void App::OnWillRemoveBrowser()
{
	if (m_browserList.GetSize() == 1 && !m_exitStarted && !IsEliteTaskbarRunning())
	{
		// The last browser window is about to be closed, which indicates that the application is
		// going to exit. Note that the exit may have already started (e.g. if there were multiple
		// windows open and the user selected the "Exit" menu item). In that case, this branch won't
		// be taken.
		OnExitStarted();
	}
}

void App::TryExit()
{
	if (!ConfirmExit())
	{
		return;
	}

	Exit();
}

bool App::ConfirmExit()
{
	if (!m_config.confirmCloseTabs)
	{
		return true;
	}

	auto numWindows = m_browserList.GetSize();

	if (numWindows == 1)
	{
		return true;
	}

	auto *browser = m_browserList.GetLastActive();
	CHECK(browser);

	std::wstring message =
		fmt::format(fmt::runtime(m_resourceLoader->LoadString(IDS_CLOSE_ALL_WINDOWS)),
			fmt::arg(L"num_windows", numWindows));
	int response =
		MessageBox(browser->GetHWND(), message.c_str(), APP_NAME, MB_ICONINFORMATION | MB_YESNO);

	if (response == IDNO)
	{
		return false;
	}

	return true;
}

void App::Exit()
{
	if (m_exitStarted)
	{
		DCHECK(false);
		return;
	}

	OnExitStarted();

	std::vector<BrowserWindow *> browsers;

	// Closing a browser window will alter the list of browsers, which is why the list is copied
	// here.
	for (auto *browser : m_browserList.GetList())
	{
		browsers.push_back(browser);
	}

	for (auto *browser : browsers)
	{
		browser->Close();
	}
}

void App::OnExitStarted()
{
	CHECK(!m_exitStarted);

	// The application is going to exit, so the settings need to be saved before the shutdown
	// begins.
	m_saveSettingsTimer.cancel();
	SaveSettings();

	m_exitStarted = true;
}

void App::SessionEnding()
{
	if (m_exitStarted)
	{
		// The application has already started exiting, so there's no need to try and save the
		// settings, since it will have already been done.
		return;
	}

	SaveSettings();
}

void App::OnEventWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	if (msg == WM_TRAYICON)
	{
		if (lParam == WM_RBUTTONUP)
		{
			POINT pt;
			GetCursorPos(&pt);
			HMENU hMenu = CreatePopupMenu();
			if (hMenu)
			{
				AppendMenuW(hMenu, MF_STRING, 1001, L"Open New Window");
				AppendMenuW(hMenu, MF_STRING, 1002, L"Quit Win32Explorer");
				SetForegroundWindow(hwnd);
				int cmd = TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_BOTTOMALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, NULL);
				if (cmd == 1001)
				{
					Win32Explorer::Create(this);
				}
				else if (cmd == 1002)
				{
					TryExit();
				}
				DestroyMenu(hMenu);
			}
		}
	}
}





