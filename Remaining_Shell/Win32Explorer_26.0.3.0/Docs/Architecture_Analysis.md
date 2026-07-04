# Architecture Analysis & Project Standards

## Coding Standards
- **Indentation**: Tabs are used for indentation.
- **Brace Style**: Allman style (braces on new lines).
- **Naming**: Hungarian notation is explicitly forbidden. Variables should use descriptive names like `title` instead of `szTitle`.

## Libraries and Tools
- **WIL (Windows Implementation Libraries)**: Used for robust Win32 resource management and error handling.
- **Boost**: Utilized for various utility functions and data structures.
- **Modern C++**: The project targets C++20/23, leveraging modern language features for safety and performance.
- **vcpkg**: Dependency management for external libraries.

## Architectural Patterns
- **COM Integration**: Extensive use of COM interfaces (`IShellView`, `IFolderView`, etc.) for Windows Shell integration.
- **Native Visuals**: Prioritizes native Windows controls and theme classes (`ExplorerBar`, `TAB`, etc.) over custom drawing to ensure the application skins naturally with the OS.
- **Decoupled Components**: The UI is separated into distinct classes like `AddressBar`, `ApplicationToolbar`, `MainTabView`, etc., to maintain modularity.
