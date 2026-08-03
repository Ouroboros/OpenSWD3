@echo off
setlocal

cd /d "%~dp0"

set "CMAKE_EXE=D:\Dev\lldb\tools\cmake\bin\cmake.exe"
set "CTEST_EXE=D:\Dev\lldb\tools\cmake\bin\ctest.exe"
set "NINJA_EXE=D:\Dev\lldb\tools\ninja\ninja.exe"
set "LLVM_BIN=D:\Dev\Compiler\LLVM\x64\bin"

if not exist "%CMAKE_EXE%" goto missing_tools
if not exist "%CTEST_EXE%" goto missing_tools
if not exist "%NINJA_EXE%" goto missing_tools
if not exist "%LLVM_BIN%\clang++.exe" goto missing_tools
if /I "%~1"=="app" if not exist "%LLVM_BIN%\clang.exe" goto missing_tools

set "PATH=%LLVM_BIN%;D:\Dev\lldb\tools\cmake\bin;D:\Dev\lldb\tools\ninja;%PATH%"

set "TARGET=%~1"
if "%TARGET%"=="" set "TARGET=core"

if /I "%TARGET%"=="core" (
    set "CONFIGURE_PRESET=core"
    set "BUILD_PRESET=core-debug"
) else if /I "%TARGET%"=="app" (
    set "CONFIGURE_PRESET=app"
    set "BUILD_PRESET=app-debug"
) else (
    echo Usage: build.bat [core^|app]
    set "BUILD_EXIT=2"
    goto finish
)

echo [OpenSWD3] Configure: %CONFIGURE_PRESET%
if /I "%TARGET%"=="app" (
    "%CMAKE_EXE%" --preset "%CONFIGURE_PRESET%" ^
        -DCMAKE_C_COMPILER:FILEPATH="%LLVM_BIN%\clang.exe" ^
        -DCMAKE_CXX_COMPILER:FILEPATH="%LLVM_BIN%\clang++.exe" ^
        -DCMAKE_MAKE_PROGRAM:FILEPATH="%NINJA_EXE%"
) else (
    "%CMAKE_EXE%" --preset "%CONFIGURE_PRESET%" ^
        -DCMAKE_CXX_COMPILER:FILEPATH="%LLVM_BIN%\clang++.exe" ^
        -DCMAKE_MAKE_PROGRAM:FILEPATH="%NINJA_EXE%"
)
if errorlevel 1 goto failed

echo [OpenSWD3] Build: %BUILD_PRESET%
"%CMAKE_EXE%" --build --preset "%BUILD_PRESET%"
if errorlevel 1 goto failed

echo [OpenSWD3] Test: Debug
"%CTEST_EXE%" --test-dir "build\%CONFIGURE_PRESET%" -C Debug --output-on-failure
if errorlevel 1 goto failed

echo [OpenSWD3] Build and tests completed successfully.
set "BUILD_EXIT=0"
goto finish

:missing_tools
echo [OpenSWD3] Required tool not found.
echo CMake: %CMAKE_EXE%
echo CTest: %CTEST_EXE%
echo Ninja: %NINJA_EXE%
echo LLVM C: %LLVM_BIN%\clang.exe
echo LLVM: %LLVM_BIN%\clang++.exe
set "BUILD_EXIT=2"
goto finish

:failed
set "BUILD_EXIT=%ERRORLEVEL%"
echo [OpenSWD3] Failed with exit code %BUILD_EXIT%.

:finish
echo.
exit /b %BUILD_EXIT%
