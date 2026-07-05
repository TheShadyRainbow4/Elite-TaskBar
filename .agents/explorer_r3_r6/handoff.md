# Handoff Report - explorer_r3_r6

This report presents a read-only exploration and analysis of the codebase for Portable Mirror Mode (R6) and Settings Synchronization & CPL Repair (R3), specifying the required registry paths, files to modify, and code proposals.

---

## 1. Observation

Exact file paths, key variables, and registry structures observed in the codebase:

1. **Win32Explorer Registry Storage Factory:**
   - Location: `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/RegistryAppStorageFactory.cpp`
   - Hardcoded HKLM calls in functions `OpenKeyForLoad` and `CreateKeyForSave`:
     ```cpp
     wil::unique_hkey RegistryAppStorageFactory::OpenKeyForLoad(const std::wstring &applicationKeyPath)
     {
         wil::unique_hkey applicationKey;
         HRESULT hr = wil::reg::open_unique_key_nothrow(HKEY_LOCAL_MACHINE, applicationKeyPath.c_str(),
             applicationKey, wil::reg::key_access::read);
         ...
     ```
   - Application registry path: `Software\Win32Explorer` (defined in `Storage.h` as `REGISTRY_APPLICATION_KEY_PATH`).

2. **Win32Explorer Config Storage Loading:**
   - Location: `Remaining_Shell/Win32Explorer_26.0.3.0/App_Source/App.cpp`
   - It checks for `config.xml` presence first, then falls back to Registry:
     ```cpp
     void App::LoadSettings(std::vector<WindowStorageData> &windows)
     {
         std::unique_ptr<AppStorage> appStorage = XmlAppStorageFactory::MaybeCreate(
             Storage::GetConfigFilePath(), Storage::OperationType::Load);
         if (appStorage) { m_savePreferencesToXmlFile = true; }
         else { appStorage = RegistryAppStorageFactory::MaybeCreate(Storage::REGISTRY_APPLICATION_KEY_PATH, Storage::OperationType::Load); }
         ...
     ```

3. **CPL Applet Stub Build Configuration:**
   - Location: `build_settings.ps1`
   - Line 27-28 compiles the stub using `EliteSettingsStub.cpp` and `settings_resources.res`:
     ```powershell
     $stubCPLCompileCmd64 = "cl.exe /LD /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\settings_cpl64.pdb`" /Fe`"$BuildDir\EliteSettings.cpl`" /Fo`"$BuildDir\SettingsObj_cpl_64_\\`" $srcSettings `"$BuildDir\settings_resources.res`" $libs /link /EXPORT:CPlApplet /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
     ```
   - `$srcSettings` is defined as:
     ```powershell
     $srcSettings = "`"$SourceDir\EliteSettingsStub.cpp`" `"$SourceDir\TaskbarProperties.cpp`""
     ```
   - `SourceFiles/EliteSettingsCpl.cpp` is never compiled but contains the `RunEmbeddedExe` routine to launch the embedded `EliteSettings.exe` (which contains the compiled WinForms UI).

4. **Explorer Replacement Registry Operations:**
   - Location: `Remaining_Shell/Win32Explorer_26.0.3.0/Shared_Libraries/SetDefaultFileManager.cpp`
   - Registry paths targeted:
     - Directory: `HKEY_CURRENT_USER\Software\Classes\Directory\shell`
     - Folder: `HKEY_CURRENT_USER\Software\Classes\Folder\shell`
   - In `GeneralOptionsPage.cpp` line 390, cleaning up default file manager values is conditional on `m_config->replaceExplorerMode`, which allows the registry state to get stuck out-of-sync:
     ```cpp
     switch (m_config->replaceExplorerMode)
     {
     case ReplaceExplorerMode::FileSystem:
         res = RemoveAsDefaultFileManagerFileSystem(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
         break;
     case ReplaceExplorerMode::All:
         res = RemoveAsDefaultFileManagerAll(SHELL_DEFAULT_INTERNAL_COMMAND_NAME);
         break;
     }
     ```

---

## 2. Logic Chain

1. **R6 Portable Mirror Mode:**
   - By parameterizing `RegistryAppStorageFactory` to accept a dynamic registry root and resolving HKLM vs HKCU depending on the setting `EnablePortableMirror` (loaded from XML or Registry), we allow the app to write configuration dynamically to HKLM.
   - Dual-saving in `App::SaveSettings()` ensures that if `EnablePortableMirror` is set, we write to `config.xml` (XML format) AND write settings to HKLM (`HKEY_LOCAL_MACHINE\Software\Win32Explorer`).
   - Adding `GetEliteRegistryRoot()` in `TaskbarProperties.cpp` checks both HKLM and HKCU for the `EnablePortableMirror` value and directs the taskbar writes (`Software\EliteSoftware\Win32Explorer\Advanced`) to the correct branch.

2. **R3 Settings Synchronization (CPL Repair):**
   - The CPL file `EliteSettings.cpl` is meant to run the WinForms UI (`EliteSettings.ps1`) to perfectly mirror all features in `EliteSettings.exe`.
   - By modifying `build_settings.ps1` to compile `SourceFiles/EliteSettingsCpl.cpp` and linking `settings_cpl.res` (which contains `1 RCDATA "EliteSettings.exe"`), double-clicking the CPL will extract and run the embedded `EliteSettings.exe` compiled from `EliteSettings.ps1`.
   - Exposing the "Replace Explorer" options inside both C++ (`TaskbarProperties.cpp`) and WinForms (`EliteSettings.ps1`) maintains a shared UI interface.

3. **Registry Restoration (Stuck Explorer Fix):**
   - The reason users get stuck in "Replace Explorer" is that the C++ UI only removes the old association if `m_config->replaceExplorerMode` is set correctly. If it goes out-of-sync, the old association persists.
   - Performing unconditional cleanup (removing both `Directory` and `Folder` keys/commands) before applying a new mode or switching to `None` resolves the issue.

---

## 3. Caveats

- Standard users might lack write permissions to `HKEY_LOCAL_MACHINE`. If the user enables "Portable Mirror", the application must be running elevated or rely on a helper tool (like `psexec64` or manifesting elevated execution) to write to HKLM.
- The path of the extracted executable for the CPL is `%TEMP%`. File operations there must have correct read/write/execute permissions.

---

## 4. Conclusion

The solution consists of:
1. Enhancing `RegistryAppStorageFactory.cpp` and `App::SaveSettings` to support toggling between HKLM and HKCU and dual-writing.
2. Linking `EliteSettingsCpl.cpp` and `settings_cpl.res` in the CPL target inside `build_settings.ps1`.
3. Adding "Replace Explorer" and "Portable Mirror" UI controls to `resources.rc` and `EliteSettings.ps1`.
4. Restoring native Explorer associations unconditionally on mode resets.

---

## 5. Verification Method

1. **Compilation:**
   - Execute `build.ps1` to build the full toolchain. Check that `EliteSettings.cpl` and `EliteSettings.exe` compile successfully.
2. **Settings Synchronisation:**
   - Open `EliteSettings.cpl` using `control.exe` or by double-clicking it. Verify that it launches the WinForms UI of `EliteSettings.ps1` identically to `EliteSettings.exe`.
3. **Registry Toggles:**
   - Enable "Portable Mirror Mode" in settings. Confirm that `config.xml` is generated with `EnablePortableMirror="yes"` and the settings are written to `HKLM\Software\Win32Explorer` and `HKLM\Software\EliteSoftware\Win32Explorer\Advanced`.
   - Enable "Replace Explorer - FileSystem" and then revert to "None". Verify that the keys `HKCU\Software\Classes\Directory\shell` and `HKCU\Software\Classes\Folder\shell` default values are completely reset and double-clicking folders opens them in native Explorer.

---

## Proposed Code Changes

### Path 1: `build_settings.ps1` (CPL repair)
Modify `$stubCPLCompileCmd64` and `$stubCPLCompileCmd86` to compile `EliteSettingsCpl.cpp` and link `settings_cpl.res`:
```powershell
# Before
$libs = "user32.lib shell32.lib shlwapi.lib comctl32.lib advapi32.lib uxtheme.lib gdi32.lib ole32.lib gdiplus.lib"
$srcSettings = "`"$SourceDir\EliteSettingsStub.cpp`" `"$SourceDir\TaskbarProperties.cpp`""
$stubCPLCompileCmd64 = "cl.exe /LD /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\settings_cpl64.pdb`" /Fe`"$BuildDir\EliteSettings.cpl`" /Fo`"$BuildDir\SettingsObj_cpl_64_\\`" $srcSettings `"$BuildDir\settings_resources.res`" $libs /link /EXPORT:CPlApplet /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"

# After
$libs = "user32.lib shell32.lib shlwapi.lib comctl32.lib advapi32.lib uxtheme.lib gdi32.lib ole32.lib gdiplus.lib"
$srcSettingsCpl = "`"$SourceDir\EliteSettingsCpl.cpp`""
$stubCPLCompileCmd64 = "cl.exe /LD /EHsc /Zi /MTd /D_DEBUG /Fd`"$BuildDir\settings_cpl64.pdb`" /Fe`"$BuildDir\EliteSettings.cpl`" /Fo`"$BuildDir\SettingsObj_cpl_64_\\`" $srcSettingsCpl `"$BuildDir\settings_cpl.res`" $libs /link /EXPORT:CPlApplet /MANIFEST:EMBED /MANIFESTINPUT:`"$SourceDir\cpl.manifest`" /MANIFESTUAC:NO"
```

### Path 2: `RegistryAppStorageFactory.cpp` (HKLM / HKCU toggle)
Modify `MaybeCreate`, `OpenKeyForLoad`, and `CreateKeyForSave` to support dynamic root keys:
```cpp
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
		SHDeleteKey(rootKey, applicationKeyPath.c_str());
		applicationKey = CreateKeyForSave(applicationKeyPath, useHKLM);
	}

	if (!applicationKey)
	{
		return nullptr;
	}

	return std::make_unique<RegistryAppStorage>(std::move(applicationKey));
}
```

### Path 3: `App.cpp` (Dual-saving settings)
Update `App::SaveSettings()` to save simultaneously if Portable Mirror is checked:
```cpp
void App::SaveSettings()
{
	CHECK(!m_exitStarted);
	std::vector<WindowStorageData> windows;
	for (const auto *browser : m_browserList.GetList())
	{
		windows.push_back(browser->GetStorageData());
	}
	DCHECK_GE(windows.size(), 1u);

	if (m_config.enablePortableMirror.get())
	{
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
		// Normal registry/xml save...
	}
}
```

### Path 4: `TaskbarProperties.cpp` (UI controls, key redirects, and stuck fix)
Add helper functions to determine the root HKEY and replace Explorer:
```cpp
HKEY GetEliteRegistryRoot() {
    HKEY hKey;
    DWORD dwValue = 0;
    DWORD cbData = sizeof(DWORD);
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\EliteSoftware\\Win32Explorer\\Advanced", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"EnablePortableMirror", NULL, NULL, (LPBYTE)&dwValue, &cbData) == ERROR_SUCCESS && dwValue == 1) {
            RegCloseKey(hKey);
            return HKEY_LOCAL_MACHINE;
        }
        RegCloseKey(hKey);
    }
    return HKEY_CURRENT_USER;
}

void SetDefaultFileManagerCPP(DWORD mode) {
    HKEY hKey;
    WCHAR exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    WCHAR* lastSlash = wcsrchr(exePath, L'\\');
    if (lastSlash) {
        wcscpy_s(lastSlash + 1, MAX_PATH - (lastSlash + 1 - exePath), L"Win32Explorer.exe");
    }

    // Unconditional cleanup to fix being stuck
    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\openinWin32Explorer");
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        DWORD cbData = sizeof(WCHAR) * 20;
        WCHAR val[20];
        if (RegQueryValueExW(hKey, L"", NULL, NULL, (LPBYTE)val, &cbData) == ERROR_SUCCESS) {
            if (wcscmp(val, L"openinWin32Explorer") == 0) {
                RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"none", sizeof(L"none"));
            }
        }
        RegCloseKey(hKey);
    }

    SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Folder\\shell\\openinWin32Explorer");
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Classes\\Folder\\shell", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        DWORD cbData = sizeof(WCHAR) * 20;
        WCHAR val[20];
        if (RegQueryValueExW(hKey, L"", NULL, NULL, (LPBYTE)val, &cbData) == ERROR_SUCCESS) {
            if (wcscmp(val, L"openinWin32Explorer") == 0) {
                RegDeleteValueW(hKey, L"");
            }
        }
        RegCloseKey(hKey);
    }

    // Write file manager registry associations based on mode (2=FileSystem, 3=All)
    if (mode == 2 || mode == 3) {
        LPCWSTR rootSubKey = (mode == 2) ? L"Software\\Classes\\Directory\\shell" : L"Software\\Classes\\Folder\\shell";
        WCHAR commandKeyPath[256];
        wsprintfW(commandKeyPath, L"%s\\openinWin32Explorer", rootSubKey);
        
        if (RegCreateKeyExW(HKEY_CURRENT_USER, commandKeyPath, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"Open in Win32Explorer", sizeof(L"Open in Win32Explorer"));
            HKEY hCmdKey;
            if (RegCreateKeyExW(hKey, L"command", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hCmdKey, NULL) == ERROR_SUCCESS) {
                WCHAR cmd[MAX_PATH + 10];
                wsprintfW(cmd, L"\"%s\" \"%%1\"", exePath);
                RegSetValueExW(hCmdKey, L"", 0, REG_SZ, (const BYTE*)cmd, (wcslen(cmd) + 1) * sizeof(WCHAR));
                RegCloseKey(hCmdKey);
            }
            RegCloseKey(hKey);
        }
        if (RegOpenKeyExW(HKEY_CURRENT_USER, rootSubKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
            RegSetValueExW(hKey, L"", 0, REG_SZ, (const BYTE*)L"openinWin32Explorer", sizeof(L"openinWin32Explorer"));
            RegCloseKey(hKey);
        }
    }
}
```

### Path 5: `EliteSettings.ps1` (PowerShell UI and dynamic paths)
Add UI controls, resolve registry paths dynamically, and save XML simultaneously:
```powershell
function Get-EliteRegPath {
    $val = (Get-ItemProperty -Path "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced" -Name "EnablePortableMirror" -ErrorAction SilentlyContinue).EnablePortableMirror
    if ($val -eq 1) { return "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced" }
    return "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced"
}

# In Save-Settings:
$portable = if ($chk_PortableMirror.Checked) { 1 } else { 0 }
Set-ItemProperty -Path "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced" -Name "EnablePortableMirror" -Value $portable -Type DWord
Set-ItemProperty -Path "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced" -Name "EnablePortableMirror" -Value $portable -Type DWord

$global:regPathElite = if ($portable -eq 1) { "HKLM:\Software\EliteSoftware\Win32Explorer\Advanced" } else { "HKCU:\Software\EliteSoftware\Win32Explorer\Advanced" }

$replaceMode = if ($rdo_ReplaceFileSys.Checked) { 2 } elseif ($rdo_ReplaceAll.Checked) { 3 } else { 1 }
Set-ItemProperty -Path $global:regPathElite -Name "ReplaceExplorerMode" -Value $replaceMode -Type DWord

# Save XML simultaneously
if ($portable -eq 1) {
    Save-ConfigXml -replaceMode $replaceMode -taskbarMode $mode -buttonWidth $width -previews $prev -portable $portable
}
```
