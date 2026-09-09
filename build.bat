@echo off
setlocal

cd /d "%~dp0"

if not defined OPENSWD3_PYTHON set "OPENSWD3_PYTHON=python"
if not defined OPENSWD3_CMAKE set "OPENSWD3_CMAKE=D:\Dev\lldb\tools\cmake\bin\cmake.exe"
if not defined OPENSWD3_CTEST set "OPENSWD3_CTEST=D:\Dev\lldb\tools\cmake\bin\ctest.exe"
if not defined OPENSWD3_NINJA set "OPENSWD3_NINJA=D:\Dev\lldb\tools\ninja\ninja.exe"
if not defined CC set "CC=D:\Dev\Compiler\LLVM\x64\bin\clang.exe"
if not defined CXX set "CXX=D:\Dev\Compiler\LLVM\x64\bin\clang++.exe"
if not defined OPENSWD3_BUILD_JOBS set "OPENSWD3_BUILD_JOBS=%NUMBER_OF_PROCESSORS%"
if not defined OPENSWD3_TEST_JOBS set "OPENSWD3_TEST_JOBS=%NUMBER_OF_PROCESSORS%"

set "PATH=D:\Dev\Compiler\LLVM\x64\bin;D:\Dev\lldb\tools\cmake\bin;D:\Dev\lldb\tools\ninja;%PATH%"

"%OPENSWD3_PYTHON%" build.py %*
exit /b %ERRORLEVEL%
