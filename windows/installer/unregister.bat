@echo off
REM Unregister (remove) the Bangla Keyboard TSF IME, both architectures. Run as Administrator.
setlocal
net session >nul 2>&1
if errorlevel 1 (
    echo This must be run as Administrator. Right-click -^> "Run as administrator".
    pause & exit /b 1
)

set "HERE=%~dp0"
set "D64=%HERE%BanglaKeyboard.dll"
if not exist "%D64%" set "D64=%HERE%..\dist\BanglaKeyboard.dll"
set "D32=%HERE%BanglaKeyboard32.dll"
if not exist "%D32%" set "D32=%HERE%..\dist\BanglaKeyboard32.dll"

if exist "%D64%" (
    echo Unregistering 64-bit: "%D64%"
    regsvr32 /u "%D64%"
)
if exist "%D32%" (
    echo Unregistering 32-bit: "%D32%"
    if exist "%SystemRoot%\SysWOW64\regsvr32.exe" (
        "%SystemRoot%\SysWOW64\regsvr32.exe" /u "%D32%"
    ) else (
        regsvr32 /u "%D32%"
    )
)
echo Done.
pause
