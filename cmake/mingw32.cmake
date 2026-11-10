# Cross-compile for i686 Windows (XP and later) with mingw-w64
# (CMakeLists.txt): programs are named *32.exe, as t/win32 expects.
#   cmake -B build-w32 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw32.cmake
# Native builds with w64devkit's i686 toolchain need no toolchain file.
# This is free and unencumbered software released into the public domain.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR i686)
set(CMAKE_C_COMPILER   i686-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER i686-w64-mingw32-g++)
