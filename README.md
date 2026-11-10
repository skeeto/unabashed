# unabashed: bash features at dash speeds

A from-scratch, faithful clone of GNU Bash 5.3 with first-class native
Windows support (CRT-free, supports Windows XP), while maintaining full
support for POSIX systems. Substantially faster than Bash, especially on
Windows, with performance matching or exceeding dash. The architecture
uses platform layers, and tests run against a virtual operating system.

The shell has two modes depending on its invocation name:

* `sh`/`ash`/`lash`: *native* mode with native paths, like [busybox-w32][]
* `bash` or any other name: *msys* mode with unix paths

unabashed is written in a C++20 subset without the C++ standard library.
Line editing is built in and it has no dependencies.

## Build

No `configure` script, no build system. On [w64devkit][]:

    $ cc -nostartfiles -fno-exceptions -O2 -o bash.exe main_win32.cpp

On POSIX systems:

    $ cc -std=c++20 -fno-exceptions -O2 -o bash main_posix.cpp

For development, CMakeLists.txt builds the shell, its tests and tools
at once, and gives a host the shell as a library (libunabashed,
unabashed.h, notes/library.md) through FetchContent or add_subdirectory.


[busybox-w32]: https://frippery.org/busybox/
[w64devkit]: https://github.com/skeeto/w64devkit
