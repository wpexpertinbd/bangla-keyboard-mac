@echo off
REM Register the Bangla Keyboard TSF IME. MUST run as Administrator.
REM Finds BanglaKeyboard.dll next to this script, else in ..\dist.
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
    echo BanglaKeyboard.dll not found next to this script or in ..\dist.
    echo Build it first (see windows\tsf\README.md), then re-run.
    pause
    exit /b 1
)

echo Registering "%DLL%" ...
regsvr32 "%DLL%"
echo.
echo If it succeeded: add the keyboard via Settings -^> Time ^& language -^>
echo Language ^& region -^> Bangla -^> Language options -^> add a keyboard
echo ("Bangla Keyboard (Unicode)"), or pick it with Win+Space.
pause
