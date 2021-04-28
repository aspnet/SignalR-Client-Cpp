@echo off

set root=%~dp0%

if not exist %root%\submodules\vcpkg\vcpkg.exe call %root%\submodules\vcpkg\bootstrap-vcpkg.bat
%root%\submodules\vcpkg\vcpkg.exe install cpprestsdk:x64-windows-static-md-141 --overlay-triplets=%root%\vcpkg-triplets