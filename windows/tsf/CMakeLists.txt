# Bangla Keyboard — TSF text service (in-proc COM DLL). Windows + Windows SDK only.
#
#   cmake -B build -A x64        (or -A Win32 for the 32-bit TIP)
#   cmake --build build --config Release
#
# Produces BanglaKeyboard.dll. Register (elevated): regsvr32 BanglaKeyboard.dll
cmake_minimum_required(VERSION 3.15)
project(BanglaKeyboard CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(BanglaKeyboard SHARED
    Globals.cpp
    TextService.cpp
    Dll.cpp
    ../engine/engine.cpp
    BanglaKeyboard.def
)
target_include_directories(BanglaKeyboard PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../engine)
target_link_libraries(BanglaKeyboard PRIVATE ole32 advapi32 user32)

if(MSVC)
    target_compile_options(BanglaKeyboard PRIVATE /utf-8 /EHsc)
    target_compile_definitions(BanglaKeyboard PRIVATE UNICODE _UNICODE)
endif()

# Non-destructive DLL smoke test (loads the DLL, exercises COM; no registry).
add_executable(loadtest loadtest.cpp)
target_link_libraries(loadtest PRIVATE ole32 oleaut32 uuid)

# 64-bit Windows wants BOTH a 64-bit and a 32-bit TIP so the IME works in 32-bit
# apps too. Configure twice (-A x64 and -A Win32) and ship both DLLs.
