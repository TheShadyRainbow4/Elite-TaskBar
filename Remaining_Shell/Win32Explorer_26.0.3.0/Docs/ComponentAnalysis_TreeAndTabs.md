# Component Analysis: Folder Tree and Tab Bar

## Folder Tree View (`ShellTreeView`)
- **Native Explorer Theme**: The tree view is updated to use the "Explorer" visual style via `SetWindowTheme(hwnd, L"Explorer", NULL)`.
- **Scroll Bar Management**: Horizontal scrolling is disabled to match the modern Explorer behavior where the tree view handles expansion without horizontal scrolling.
- **Integration**: It integrates with the shell to provide a hierarchical view of the filesystem.

## Tab Bar System (`MainTabView`)
- **Native Styling**: Updated to remove `TCS_EX_FLATBUTTONS` and use native `TAB` theme parts.
- **Visuals**: Tabs are designed to be skinned by the Windows system rather than custom-drawn, ensuring consistency with the OS.
- **Tab Backing**: Uses `TABP_PANE` for the background area of the tabs.
