@echo off
rem Builds the Windows program natively and packs what a player needs into build\OpenSWD3-<date>-<commit>.zip:
rem swd3r.exe, the asset folder (the font and the resources extracted from swd32.exe) and the settings template
rem swd3r.toml, all in one OpenSWD3 folder.
rem Needs LLVM (clang-cl, lld-link, llvm-lib, llvm-rc, llvm-mt; SWD3_LLVM, by default D:\Dev\Compiler\LLVM\x64\bin),
rem Visual Studio 2022 with the C++ tools and its CMake and Ninja (the Windows SDK and the MSVC libraries), the
rem dependencies of scripts\deps and third_party\asset (docs\building.md). Git, when found, adds the commit to the name.
rem Usage: scripts\package.bat
setlocal EnableExtensions

if not defined SWD3_LLVM set "SWD3_LLVM=D:\Dev\Compiler\LLVM\x64\bin"
cd /d "%~dp0.."
set "ROOT=%CD%"
set "BUILD=%ROOT%\build\windows-native"
set "STAGE=%ROOT%\build\package"

if not exist "%SWD3_LLVM%\clang-cl.exe" (
    echo clang-cl.exe not found in %SWD3_LLVM% ^(set SWD3_LLVM^)
    exit /b 1
)

if not exist "%ROOT%\third_party\asset\font.bin" (
    echo third_party\asset\font.bin missing ^(see docs\building.md^)
    exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSDIR="
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"

if not defined VSDIR (
    echo Visual Studio with the C++ tools not found
    exit /b 1
)

call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
set "PATH=%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin;%VSDIR%\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja;%PATH%"
set "LLVM=%SWD3_LLVM:\=/%"

cmake -S "%ROOT%" -B "%BUILD%" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo ^
    -DCMAKE_C_COMPILER="%LLVM%/clang-cl.exe" -DCMAKE_CXX_COMPILER="%LLVM%/clang-cl.exe" ^
    -DCMAKE_LINKER="%LLVM%/lld-link.exe" -DCMAKE_AR="%LLVM%/llvm-lib.exe" ^
    -DCMAKE_RC_COMPILER="%LLVM%/llvm-rc.exe" -DCMAKE_MT="%LLVM%/llvm-mt.exe" ^
    -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded || exit /b 1
cmake --build "%BUILD%" --target swd3r || exit /b 1

set "DAY="
for /f %%i in ('powershell -NoProfile -Command "Get-Date -Format yyyyMMdd"') do set "DAY=%%i"
set "COMMIT="
for /f %%i in ('git -C "%ROOT%" rev-parse --short HEAD 2^>nul') do set "COMMIT=-%%i"
set "NAME=OpenSWD3-%DAY%%COMMIT%"

if exist "%STAGE%" rmdir /s /q "%STAGE%"
mkdir "%STAGE%\OpenSWD3" || exit /b 1
copy /y "%BUILD%\swd3r.exe" "%STAGE%\OpenSWD3\" >nul || exit /b 1
copy /y "%ROOT%\swd3r.toml" "%STAGE%\OpenSWD3\" >nul || exit /b 1
xcopy /e /i /q /y "%BUILD%\asset" "%STAGE%\OpenSWD3\asset" >nul || exit /b 1
if exist "%ROOT%\build\%NAME%.zip" del "%ROOT%\build\%NAME%.zip"
"%SystemRoot%\System32\tar.exe" -a -c -f "%ROOT%\build\%NAME%.zip" -C "%STAGE%" OpenSWD3 || exit /b 1
rmdir /s /q "%STAGE%"
echo %ROOT%\build\%NAME%.zip
