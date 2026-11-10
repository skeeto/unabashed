# Cross-compile for x64 Windows with mingw-w64 (CMakeLists.txt):
#   cmake -B build-w64 -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64.cmake
# Native builds with w64devkit need no toolchain file.
# This is free and unencumbered software released into the public domain.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
