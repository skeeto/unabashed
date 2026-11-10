// Win32 platform layer: the executable, no CRT, no headers, runs on
// Windows XP and later
// $ g++ -std=c++20 -O2 -s -nostartfiles -static-libgcc
//       -o unabashed.exe main_win32.cpp
// The i686 build also takes -Wl,--large-address-aware (4 GiB on x64
// Windows) and subsystem version 5.01 for XP: see the Makefile.
// This is free and unencumbered software released into the public domain.
#include "src/win32.cpp"
