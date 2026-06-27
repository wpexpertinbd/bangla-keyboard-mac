@echo off
REM Unregister (remove) the Bangla Keyboard TSF IME. MUST run as Administrator.
setlocal
net session >nul 2>&1
if errorlevel 1 (
    echo This must be run as Administrator. Right-click -^> "Run as administrator".
    pause
    exit /b 1
)

set "DLL=%~dp0BanglaKeyboard.dll"
if not exist "%DLL%" set "DLL=%~dp0..\dist\BanglaKeyboard.dll"
if not exist "%DLL%" (
    echo BanglaKeyboard.dll not found. If you moved it, run: regsvr32 /u ^<path^>
    pause
    exit /b 1
)

echo Unregistering "%DLL%" ...
regsvr32 /u "%DLL%"
pause
