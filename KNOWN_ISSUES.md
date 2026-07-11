# Known Issues & Regressions (To Be Fixed Next Session)

## 1. Quick Launch Rendering
- **Issue:** The Quick Launch toolbar is rendering over the system tray area and has a solid background (violating native styling rules).
- **Expected Behavior:** It must render *before* the active programs (immediately to the right of the Start button). It should have no toolbar name, no labels (just icons), and it must be draggable/stretchable exactly like the Windows Vista Quick Launch.

## 2. Multi-Monitor Flyouts & Start Menu
- **Issue:** The Start Menu and other flyouts (like volume/network from the tray) always open on the primary monitor, regardless of which monitor's taskbar was clicked.
- **Expected Behavior:** When a button or icon is clicked on Monitor 2, the resulting menu or flyout MUST open on Monitor 2. The taskbar must calculate and pass the correct screen coordinates to OpenShell or the native flyout APIs.

## 3. Custom Start Menu Crash
- **Issue:** Launching the custom start menu results in a crash with an error message.
- **Expected Behavior:** The start menu should launch successfully without crashing, opening the OpenShell menu on the correct monitor.

## 4. Desktop Rendering & Interaction
- **Issue:** The custom desktop window currently draws a blank/black background instead of using the native Windows wallpaper settings. Furthermore, once opened, there is no way to close the desktop window.
- **Expected Behavior:** The desktop must fetch and render the native Windows wallpaper perfectly. It must also have a standard mechanism to be closed if needed.

## 5. Win32Explorer Blank Thumbnails
- **Issue:** The newly added thumbnail views inside `Win32Explorer` are completely blank, displaying no icons or thumbnails.
- **Expected Behavior:** The item drawing logic for folder views must be repaired to correctly extract and render file icons and thumbnails.

## 6. Tray Icon State Changes (NIM_MODIFY)
- **Issue:** Apps that rapidly change their tray icons (like Task Manager CPU usage) were creating hundreds of duplicate icons instead of updating the existing one.
- **Status:** A fix utilizing `guidItem` and `bUseGUID` was drafted, but it requires final testing to ensure state changes (`NIM_MODIFY`) correctly update existing tray icons without spawning duplicates.

## 7. Remaining Pending Features
- **Full Deskband Support & Folders**: Native implementation of QuickLaunch, toolbars, deskbands and folder toolbars, allowing different toolbars per taskbar monitor.
- **Explorer Integration**: File browser compatibility must remain fully functional while running as shell.
- **Native Windows 7 Flyout Mode**: Restore and integrate native Windows 7 Notification Area flyout UI functionality.
- **Legacy Tray Mode**: Implement ReactOS-style classic linear tray logic as an alternate selectable mode.
- **Settings Menu Expansion**: Populate all tabs with all settings, implementing switching between OpenShell, Native Menu, and custom combinations based on triggers.
