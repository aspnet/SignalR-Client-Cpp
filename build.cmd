@echo off

set root=%~dp0%
set build=%root%build
set output=%build%\output
set output_lib=%output%\lib
set output_dll=%output%\dll
set output_include=%output%\include

if exist %output_lib%\x64 rmdir /s/q %output_lib%\x64
if exist %output_dll%\x64 rmdir /s/q %output_dll%\x64

robocopy /s /njh /njs include %output_include%
if %errorlevel% geq 8 exit /b %errorlevel%

if exist %build%\CMakeCache.txt del /F %build%\CMakeCache.txt
if %errorlevel% neq 0 exit /b %errorlevel%

cmake %root% -G "Visual Studio 15 2017" -B %build% -A x64 -DCMAKE_TOOLCHAIN_FILE="%root%\submodules\vcpkg\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static-md-141 -DBUILD_TESTING=false -DUSE_CPPRESTSDK=true -DINCLUDE_JSONCPP=true 
if %errorlevel% neq 0 exit /b %errorlevel%

call :build_all x64 Debug
if %errorlevel% neq 0 exit /b %errorlevel%
call :build_all x64 Release
if %errorlevel% neq 0 exit /b %errorlevel%

exit /b 0

REM ===================================================================================================================

:build_all
echo Building %1\%2:

call :build_project %build%\microsoft-signalr.sln microsoft-signalr %1 %2
if %errorlevel% neq 0 exit /b %errorlevel%
robocopy /njh /njs %build%\bin\%2 %output_lib%\%1\%2 *.lib
if %errorlevel% geq 8 exit /b %errorlevel%
robocopy /njh /njs %build%\bin\%2 %output_dll%\%1\%2 *.dll *.pdb
if %errorlevel% geq 8 exit /b %errorlevel%

exit /b 0

REM ===================================================================================================================

:build_project
msbuild /nologo /verbosity:minimal /m /target:%2 /p:Configuration=%4 /p:Platform=%3 %1
exit /b %errorlevel%



