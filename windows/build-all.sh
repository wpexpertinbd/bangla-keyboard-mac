#!/usr/bin/env bash
# Build every Windows artifact into windows/dist/ with MinGW g++.
# Set GXX to your g++ if it isn't on PATH, e.g.:
#   GXX=/path/to/w64devkit/bin/g++ ./build-all.sh
# (w64devkit ships the TSF headers; the DLL also builds with MSVC + Windows SDK.)
set -euo pipefail
cd "$(dirname "$0")"
GXX="${GXX:-g++}"
mkdir -p dist

echo "[1/4] enginetest.exe  (SPEC corpus)"
"$GXX" -std=c++17 -O2 -static -finput-charset=UTF-8 engine/test.cpp engine/engine.cpp -o dist/enginetest.exe
echo "[2/4] bangla-demo.exe (interactive demo)"
"$GXX" -std=c++17 -O2 -static -finput-charset=UTF-8 engine/demo.cpp engine/engine.cpp -o dist/bangla-demo.exe
echo "[3/4] BanglaKeyboard.dll (TSF IME, x64)"
"$GXX" -std=c++17 -O2 -shared -static -static-libgcc -static-libstdc++ -DUNICODE -D_UNICODE \
  engine/engine.cpp tsf/Globals.cpp tsf/TextService.cpp tsf/Dll.cpp tsf/BanglaKeyboard.def \
  -Itsf -Iengine -o dist/BanglaKeyboard.dll -lole32 -loleaut32 -luuid -ladvapi32 -luser32
echo "[4/4] loadtest.exe    (non-destructive DLL check)"
"$GXX" -std=c++17 -O2 -static tsf/loadtest.cpp -o dist/loadtest.exe -lole32 -loleaut32 -luuid

echo "Done -> windows/dist/"
