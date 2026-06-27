@echo off
REM Build every Windows artifact into windows\dist\ with MinGW g++.
REM Needs g++ on PATH. Recommended: w64devkit (github.com/skeeto/w64devkit) —
REM a single-zip portable MinGW that ships the TSF headers (msctf.h).
REM (The TSF DLL also builds with MSVC + Windows SDK; see tsf\README.md.)
setlocal
cd /d "%~dp0"
where g++ >nul 2>nul || (echo Need MinGW g++ on PATH ^(w64devkit recommended^). & exit /b 1)
if not exist dist mkdir dist

echo [1/4] enginetest.exe  (SPEC corpus)
g++ -std=c++17 -O2 -static -finput-charset=UTF-8 engine\test.cpp engine\engine.cpp -o dist\enginetest.exe || goto :fail
echo [2/4] bangla-demo.exe (interactive demo)
g++ -std=c++17 -O2 -static -finput-charset=UTF-8 engine\demo.cpp engine\engine.cpp -o dist\bangla-demo.exe || goto :fail
echo [3/4] BanglaKeyboard.dll (TSF IME, x64)
g++ -std=c++17 -O2 -shared -static -static-libgcc -static-libstdc++ -DUNICODE -D_UNICODE engine\engine.cpp tsf\Globals.cpp tsf\TextService.cpp tsf\Dll.cpp tsf\BanglaKeyboard.def -Itsf -Iengine -o dist\BanglaKeyboard.dll -lole32 -loleaut32 -luuid -ladvapi32 -luser32 || goto :fail
echo [4/4] loadtest.exe    (non-destructive DLL check)
g++ -std=c++17 -O2 -static tsf\loadtest.cpp -o dist\loadtest.exe -lole32 -loleaut32 -luuid || goto :fail

echo.
echo Done. Artifacts in windows\dist\
echo   enginetest.exe       run for the 13/13 corpus check
echo   bangla-demo.exe      type Bangla in a console (Esc to quit)
echo   BanglaKeyboard.dll   the IME (register with installer\register.bat, as admin)
echo   loadtest.exe         optional: verifies the DLL loads (no registry changes)
exit /b 0
:fail
echo BUILD FAILED
exit /b 1
