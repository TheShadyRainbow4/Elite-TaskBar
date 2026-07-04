# Requirements

- Visual Studio 2019/2022 (2022 recommended). Install the "Desktop development with C++" workload.
- Windows 10 SDK (most recent version recommended)
- (Optional) To build for ARM64, the following Visual Studio components should be installed:
    - MSVC v143 - VS 2022 C++ ARM64 build tools (Latest)
- (Optional) To build with Clang (using the Debug-Clang/Release-Clang solution configurations), the following Visual Studio components should be installed:
    - C++ Clang tools for Windows
- (Optional) To build the installer, WiX 3.11 and the relevant Visual Studio extension should be installed. Download links for both of those items can be found on the [WiX website](https://wixtoolset.org/docs/wix3/).

# Setup

[vcpkg](https://vcpkg.io/) is used to manage dependencies. Before you can build Win32Explorer, you'll first need to initialize vcpkg:

- Clone this repo with submodules enabled:
```
git clone --recurse-submodules https://github.com/EliteSoftwareTech/win32explorer.git
cd win32explorer
```
- Run the vcpkg bootstrapper:
    - cmd/PowerShell: `.\External_Dependencies\\vcpkg\bootstrap-vcpkg.bat`
    - git bash/posix shell: `./Win32Explorer/ThirdParty/vcpkg/bootstrap-vcpkg.sh`


The relevant packages should then be automatically installed during the first build.

# Compiling

Open `Project_Core\\Win32Explorer.sln`. From within Visual Studio, select `Debug` > `Start Without Debugging` to compile and run the program.

# Translations

Building the program in release mode will also build all of the translations. The resulting DLLs can then be used with Win32Explorer.

# Tests

The `TestWin32Explorer` project contains unit tests for the solution as a whole. The GoogleTest package is installed via vcpkg, so provided vcpkg has been initialized and you've been able to build the solution, you should just need to build the `TestWin32Explorer` project, then run the tests via the Visual Studio Test Explorer.

Note that the `TestHelper` project is older and is in the process of being removed. It doesn't currently compile and shouldn't be used.


