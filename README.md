# ASP.NET Core SignalR C++ Client

This project is part of ASP.NET Core. You can find samples, documentation and getting started instructions for ASP.NET Core at https://github.com/aspnet/AspNetCore.

Use https://github.com/aspnet/AspNetCore/issues for issues with this project.

### Build on Windows ###
```powershell
PS> git submodule update --init
PS> .\submodules\vcpkg\bootstrap-vcpkg.bat
PS> .\submodules\vcpkg\vcpkg.exe install cpprestsdk:x64-windows
PS> mkdir build.release
PS> cd build.release
PS> cmake .. -A x64 -DCMAKE_TOOLCHAIN_FILE="..\submodules\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release
PS> cmake --build . --config Release
```
Output will be in `build.release\bin\Release\`

### Build on Mac ###
```bash
$ git submodule update --init
$ brew install gcc6
$ ./submodules/vcpkg/bootstrap-vcpkg.sh
$ ./submodules/vcpkg/vcpkg install cpprestsdk
$ mkdir build.release
$ cd build.release
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../submodules/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config Release
```
Output will be in `build.release/bin/`

### Build on Linux ###

```bash
$ git submodule update --init
$ ./submodules/vcpkg/bootstrap-vcpkg.sh
$ ./submodules/vcpkg/vcpkg install cpprestsdk boost-system boost-chrono boost-thread
$ mkdir build.release
$ cd build.release
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../submodules/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config Release
```
Output will be in `build.release/bin/`