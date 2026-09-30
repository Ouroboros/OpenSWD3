@echo off
setlocal
cd /d "%~dp0..\..\.." || exit /b 1
set "TMP=%CD%\build\tmp\runtime"
set "TEMP=%TMP%"
set "TMPDIR=%TMP%"
set "PYINSTALLER_CONFIG_DIR=%TMP%\oracle-pyinstaller-config"
set "PYTHONPYCACHEPREFIX=%TMP%\oracle-windows-pycache"
if not exist "%TMP%" mkdir "%TMP%" || exit /b 1
py -3 -m PyInstaller --noconfirm --noupx --onedir --name battle-actor-frame-oracle-v2 --add-data "%CD%\analysis\tools\battle-actor-frame-oracle\agent.js;." --add-data "%CD%\analysis\tools\battle-actor-frame-oracle\flags_bridge.c;." --specpath build\tmp\runtime --workpath build\tmp\runtime\oracle-pyinstaller-v2 --distpath build\vm analysis\tools\battle-actor-frame-oracle\capture.py
if errorlevel 1 exit /b 1
copy /y analysis\tools\battle-actor-frame-oracle\README.md build\vm\battle-actor-frame-oracle-v2\README.md >nul
exit /b %errorlevel%
