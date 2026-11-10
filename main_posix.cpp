// POSIX platform layer (Linux, macOS): the executable
// $ g++ -std=c++20 -O2 -fno-exceptions -fno-rtti -fno-threadsafe-statics \n//       -nostdlib++ -o unabashed main_posix.cpp
// (only libc: libstdc++ would cost half of startup, for nothing)
// This is free and unencumbered software released into the public domain.
#include "src/posix.cpp"
