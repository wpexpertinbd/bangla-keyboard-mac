@echo off
REM Register the Bangla Keyboard TSF IME (64-bit and/or 32-bit). Run as Administrator.
REM Registers BanglaKeyboard.dll with the native regsvr32 (the 64-bit one on x64
REM Windows) and BanglaKeyboard32.dll with the 32-bit regsvr32 (SysWOW64), so the
REM IME works in both 64-bit and 32-bit apps. Both DLLs share one CLSID.
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

set "DID=0"
if exist "%D64%" (
    echo Registering 64-bit: "%D64%"
    regsvr32 "%D64%"
    set "DID=1"
)
if exist "%D32%" (
    echo Registering 32-bit: "%D32%"
    if exist "%SystemRoot%\SysWOW64\regsvr32.exe" (
        "%SystemRoot%\SysWOW64\regsvr32.exe" "%D32%"
    ) else (
        REM 32-bit Windows: the only regsvr32 is already 32-bit.
        regsvr32 "%D32%"
    )
    set "DID=1"
)

if "%DID%"=="0" (
    echo No BanglaKeyboard.dll / BanglaKeyboard32.dll found. Build first ^(windows\build-all.bat^).
    pause & exit /b 1
)

echo.
echo Done. Add the keyboard: Settings -^> Time ^& language -^> Language ^& region
echo -^> Bangla -^> Language options -^> add a keyboard ^("Bangla Keyboard (Unicode)"^),
echo or switch with Win+Space.
pause
