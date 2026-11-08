# unabashed

A from-scratch, faithful clone of GNU bash 5.3, written for w64devkit:
Windows first class (CRT-free, Windows XP and later), with POSIX and
fully virtual platform layers for testing. Work in progress; see
`notes/plan.md` for the design and milestones.

## Build

Each build is a single command (see the header of each `main_*.cpp`);
the Makefile is only a convenience.

    $ g++ -std=c++20 -O2 -s -nostartfiles -static-libgcc -Wl,--stack,0x1000000 -o unabashed.exe main_win32.cpp
    $ g++ -std=c++20 -O2 -o unabashed main_posix.cpp
    $ make tests.exe && ./tests.exe     # Windows host, virtual OS
    $ make tests && ./tests             # Linux host, virtual OS
    $ make fuzz                         # AFL++, virtual OS only

## Layout

| File             | Purpose                                         |
|------------------|-------------------------------------------------|
| `base.cpp`       | types, arenas, strings, slices, maps            |
| `unabashed.cpp`  | platform API table and the shell core           |
| `wide.cpp`       | UTF-16/WTF-8, command lines, CRT fds, PE sniff  |
| `fibers.cpp`     | fibers for the platform layers                  |
| `main_win32.cpp` | Win32 layer                                     |
| `main_posix.cpp` | POSIX layer                                     |
| `main_test.cpp`  | virtual OS and unit tests                       |
| `main_fuzz.cpp`  | AFL++ harness on the virtual OS                 |
| `notes/`         | research, design reviews, and the plan          |

Bash's source and test suite are used only as an external reference and
oracle. None of it is copied here: `bash-5.3.tar.gz` is ignored by git.

This is free and unencumbered software released into the public domain.
