## 2026-07-06T03:08:49Z
Investigate the existing wallpaper personalization logic in C:\Users\Administrator\Desktop\Elite-TaskBar\SourceFiles\DesktopWindow.cpp and C:\Users\Administrator\Desktop\Elite-TaskBar\SourceFiles\TaskbarProperties.cpp (and any other relevant files) and recommend a fix strategy for the following requirements:
1. Keep the custom `.theme` directory parsing logic in TaskbarProperties.cpp intact to show previews of theme icons (do NOT remove it).
2. Rip out custom wallpaper slideshow timers, directory scans, and custom wallpaper file configurations from DesktopWindow.cpp and TaskbarProperties.cpp.
3. Desktop wallpaper rendering must strictly read/write directly from/to the native Windows registry settings under HKCU\Control Panel\Desktop\Wallpaper, WallpaperStyle, and TileWallpaper (and system APIs like SPI_GETDESKWALLPAPER, SPI_SETDESKWALLPAPER).
4. If the style is "22", render in Span mode (stretched across the virtual screen). For other styles (Fill "10", Fit "6", Stretch "2", Tile "0"/TileWallpaper="1", Center "0"/TileWallpaper="0"), render the wallpaper on each monitor individually using EnumDisplayMonitors.
5. In the settings UI (TaskbarProperties.cpp), setting a custom wallpaper must write to and change the native Windows personalization settings.
6. Respect changes in native Windows Personalization settings dynamically by handling WM_SETTINGCHANGE in ProgmanWndProc (reload wallpaper, clear cache, and force repaint).
