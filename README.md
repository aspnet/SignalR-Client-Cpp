# ASP.NET Core SignalR C++ Client

This project is part of ASP.NET Core. You can find samples, documentation and getting started instructions for ASP.NET Core at https://github.com/aspnet/AspNetCore.

Use https://github.com/aspnet/AspNetCore/issues for issues with this project.

## Install this library

There are multiple ways to build this library

* Use [vcpkg](https://github.com/microsoft/vcpkg) and install the library with `vcpkg install microsoft-signalr`
* Build from [command line](#command-line-build)
* Build in Visual Studio (must have the "Desktop Development with C++" workload) by selecting the "Open a local folder" option and selecting the repository root folder

## Command line build

Below are instructions to build on different OS's. You can also use the following options to customize the build:

| Command line | Description | Default value |
| --- | --- | --- |
| -DBUILD_SAMPLES | Build the included sample project | false |
| -DBUILD_TESTING | Builds the test project | true |
| -DUSE_CPPRESTSDK | Includes the CppRestSDK (default http stack) | false |
| -DWERROR | Enables warnings as errors | true |
| -DWALL | Enables all warnings | true |

### Build on Windows ###
```powershell
PS> git submodule update --init
PS> .\submodules\vcpkg\bootstrap-vcpkg.bat
PS> .\submodules\vcpkg\vcpkg.exe install cpprestsdk:x64-windows
PS> mkdir build.release
PS> cd build.release
PS> cmake .. -A x64 -DCMAKE_TOOLCHAIN_FILE="..\submodules\vcpkg\scripts\buildsystems\vcpkg.cmake" -DCMAKE_BUILD_TYPE=Release -DUSE_CPPRESTSDK=true
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
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../submodules/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CPPRESTSDK=true
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
$ cmake .. -DCMAKE_TOOLCHAIN_FILE=../submodules/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release -DUSE_CPPRESTSDK=true
$ cmake --build . --config Release
```
Output will be in `build.release/bin/`

## Example usage

```cpp
#include "signalrclient/hub_connection.h"
#include "signalrclient/hub_connection_builder.h"
#include "signalrclient/signalr_value.h"

std::promise<void> start_task;
signalr::hub_connection connection = signalr::hub_connection_builder::create("http://localhost:5000/hub").build();

connection.on("Echo", [](const signalr::value& m)
{
    std::cout << m.as_array()[0].as_string() << std::endl;
});

connection.start([&start_task](std::exception_ptr exception) {
    start_task.set_value();
});

start_task.get_future().get();

std::promise<void> send_task;
std::vector<signalr::value> arr { "Hello world" };
signalr::value arg(arr);
connection.invoke("Echo", arg, [&send_task](const signalr::value& value, std::exception_ptr exception) {
    send_task.set_value();
});

send_task.get_future().get();

std::promise<void> stop_task;
connection.stop([&stop_task](std::exception_ptr exception) {
    stop_task.set_value();
});

stop_task.get_future().get();
```

### Example CMake file

```
cmake_minimum_required (VERSION 3.5)
project (signalrclient-sample)

find_path(SIGNALR_INCLUDE_DIR signalrclient/hub_connection.h)
include_directories(${SIGNALR_INCLUDE_DIR})

find_library(SIGNALR_LIBRARY NAMES signalrclient PATHS {SIGNALR_INCLUDE_DIR} REQUIRED)
link_libraries(${SIGNALR_LIBRARY})

add_executable (sample sample.cpp)
```
