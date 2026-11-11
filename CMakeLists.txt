# unabashed: the shell, its tests, and libunabashed, the shell as a
# library (unabashed.h, notes/library.md).
#
# No build system is needed for a plain build: each executable is one
# compiler command, given in the header of its root, the main_*.cpp for
# the platform (main_linux.cpp, main_darwin.cpp, main_win32.cpp). CMake
# builds everything at once, and gives hosts the library.
#
# As a subproject (FetchContent or add_subdirectory) it defines only the
# library, target libunabashed with the alias unabashed::unabashed: an
# object library of the platform's libunabashed_*.cpp, with unabashed.h
# on its include path, needing GCC or Clang.
#
#   FetchContent_Declare(unabashed URL ... URL_HASH SHA256=...)
#   FetchContent_MakeAvailable(unabashed)
#   target_link_libraries(host PRIVATE unabashed::unabashed)
#
# It takes the host's build type. A host built with AddressSanitizer
# builds the library with it too, so that its fibers are annotated:
#
#   target_compile_options(libunabashed PRIVATE -fsanitize=address)
#
# At the top level it builds the shell and its tools, each with its own
# flags whatever the build type:
#
#   cmake -B build -G Ninja && cmake --build build
#   cmake --build build --target check      # the tests (vos)
#   cmake --build build --target check-lib  # the library's objects and hosts
#   cmake --build build --target dist       # HEAD as unabashed-DATE-HASH.tar.gz
#
# Windows, cross or native, in its own build directory:
#   -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64.cmake  x64: unabashed.exe, ubhost.exe ...
#   -DCMAKE_TOOLCHAIN_FILE=cmake/mingw32.cmake  i686 for XP: unabashed32.exe,
#                                               ubhost32.exe, ubtiny.exe ...
# Fuzzing (AFL++, main_fuzz.cpp):
#   -DCMAKE_CXX_COMPILER=afl-clang-fast++ -DUNABASHED_FUZZ=ON
#
# This is free and unencumbered software released into the public domain.
cmake_minimum_required(VERSION 3.21)
project(unabashed LANGUAGES C CXX)

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(FATAL_ERROR "unabashed needs GCC or Clang (MinGW on Windows)")
endif()

# The platform: its roots are main_${UNABASHED_OS}.cpp and
# libunabashed_${UNABASHED_OS}.cpp
if(WIN32)
  set(UNABASHED_OS win32)
elseif(APPLE)
  set(UNABASHED_OS darwin)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(UNABASHED_OS linux)
else()
  message(FATAL_ERROR "unabashed builds for Linux, macOS and Windows")
endif()

set(UNABASHED_FLAGS -fno-exceptions -fno-rtti -fno-threadsafe-statics
                    -fno-asynchronous-unwind-tables)

add_library(libunabashed OBJECT libunabashed_${UNABASHED_OS}.cpp)
add_library(unabashed::unabashed ALIAS libunabashed)
set_target_properties(libunabashed PROPERTIES CXX_STANDARD 20
                      CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF)
target_compile_options(libunabashed PRIVATE ${UNABASHED_FLAGS})
target_include_directories(libunabashed INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
if(WIN32)
  # Plus memcpy, memmove, memset, memcmp and strlen from the host
  target_link_libraries(libunabashed INTERFACE kernel32)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(libunabashed INTERFACE Threads::Threads)
endif()

if(NOT PROJECT_IS_TOP_LEVEL)
  return()
endif()


# The shell and its tools

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
# Each program names what it links: not CMake's default Windows libraries
set(CMAKE_C_STANDARD_LIBRARIES "")
set(CMAKE_CXX_STANDARD_LIBRARIES "")

set(WARN -Wall -Wextra -Wconversion -Wno-sign-conversion
         -Wno-unused-parameter -Wno-unused-function)
set(DEBUG -g3 -fsanitize=undefined -fsanitize-trap)
set(CWARN -Wall -Wextra -Wno-missing-field-initializers)
set(BITS32 OFF)
if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(BITS32 ON)
endif()
# 32-bit Windows programs load on XP (subsystem 5.1); the i686 shell's
# programs are named *32.exe, so that t/win32 finds the 32-bit sibling.
set(XP -Wl,--major-subsystem-version=5,--minor-subsystem-version=1)
set(SUFFIX "")
if(BITS32)
  set(SUFFIX 32)
endif()
# A shell's options go to the link as well, as in its single compiler
# command: on macOS the linker's output depends on -O.
function(shell name source)  # name, then compile options
  add_executable(${name} ${source})
  target_compile_options(${name} PRIVATE ${UNABASHED_FLAGS} ${WARN} ${ARGN})
  target_link_options(${name} PRIVATE ${ARGN})
  if(WIN32)
    target_link_options(${name} PRIVATE -nostartfiles -static-libgcc)
    if(BITS32)
      target_link_options(${name} PRIVATE ${XP} -Wl,--large-address-aware)
    endif()
  endif()
endfunction()

shell(unabashed main_${UNABASHED_OS}.cpp -O2)
shell(debug main_${UNABASHED_OS}.cpp ${DEBUG})
target_link_options(unabashed PRIVATE -s)
if(NOT WIN32)
  target_link_options(unabashed PRIVATE -nostdlib++)
endif()
set_target_properties(unabashed PROPERTIES OUTPUT_NAME unabashed${SUFFIX})
set_target_properties(debug PROPERTIES OUTPUT_NAME debug${SUFFIX})

add_executable(tests main_test.cpp)
target_compile_options(tests PRIVATE ${UNABASHED_FLAGS} ${WARN} ${DEBUG} -Og)
target_link_options(tests PRIVATE ${DEBUG} -Og)
if(WIN32)
  # Room for bash's recursion depths in oracle runs (main_test.cpp)
  target_link_options(tests PRIVATE -Wl,--stack,0x2000000)
endif()
set_target_properties(tests PROPERTIES OUTPUT_NAME tests${SUFFIX})
if(BITS32)
  # Not part of the i686 build, where it was never built: its vos and
  # fakes warn there (-Wconversion)
  set_target_properties(tests PROPERTIES EXCLUDE_FROM_ALL ON)
endif()

option(UNABASHED_FUZZ "Build fuzz from main_fuzz.cpp (AFL++ compiler)" OFF)
if(UNABASHED_FUZZ)
  add_executable(fuzz main_fuzz.cpp)
  target_compile_options(fuzz PRIVATE ${UNABASHED_FLAGS} ${DEBUG})
  target_link_options(fuzz PRIVATE ${DEBUG})
endif()

if(WIN32)
  # tools/argv.cpp keeps the CRT startup on purpose: it shows how
  # msvcrt.dll splits a command line (tools/README.md)
  add_executable(argv tools/argv.cpp)
  target_compile_options(argv PRIVATE ${WARN} -O2 -municode)
  target_link_options(argv PRIVATE -O2 -municode -s)
  set_target_properties(argv PROPERTIES OUTPUT_NAME argv${SUFFIX})
endif()


# The library's checks: the objects (tools/check-lib), at -O2 and -O0,
# and the hosts: ubhost (tools/ubhost.c), a drop-in for the shell with
# --check; ubcollide, a C++ host that defines the shell's own names;
# ubtiny, a CRT-free i686 host with no root or self.

target_compile_options(libunabashed PRIVATE ${WARN} -O2)
add_library(libunabashed-O0 OBJECT libunabashed_${UNABASHED_OS}.cpp)
target_compile_options(libunabashed-O0 PRIVATE ${UNABASHED_FLAGS} ${WARN} -O0)

# The C hosts link the object itself, not the target, so that CMake
# links them as C, with no C++ runtime: they show that the library needs
# none.
add_executable(ubhost tools/ubhost.c)
target_link_libraries(ubhost PRIVATE $<TARGET_OBJECTS:libunabashed>)
add_dependencies(ubhost libunabashed)
target_compile_options(ubhost PRIVATE -O2 ${CWARN})
target_link_options(ubhost PRIVATE -O2)
set_target_properties(ubhost PROPERTIES OUTPUT_NAME ubhost${SUFFIX})
if(WIN32)
  # No C runtime: ubhostw.c brings the mem* and strlen the library needs
  target_link_options(ubhost PRIVATE -nostartfiles -nostdlib)
  target_link_libraries(ubhost PRIVATE kernel32 gcc)
  if(BITS32)
    target_link_options(ubhost PRIVATE ${XP} -Wl,--large-address-aware)
    add_executable(ubtiny tools/ubtiny.c)
    target_link_libraries(ubtiny PRIVATE $<TARGET_OBJECTS:libunabashed>)
    add_dependencies(ubtiny libunabashed)
    target_compile_options(ubtiny PRIVATE -O2 -Wall -Wextra)
    target_link_options(ubtiny PRIVATE -O2 -nostartfiles -nostdlib ${XP})
    target_link_libraries(ubtiny PRIVATE kernel32 gcc)
  endif()
else()
  target_link_libraries(ubhost PRIVATE Threads::Threads)
  add_executable(ubcollide tools/ubcollide.cpp)
  target_compile_options(ubcollide PRIVATE ${UNABASHED_FLAGS})  # unoptimized, as its object
  target_link_options(ubcollide PRIVATE -nostdlib++)
  target_link_libraries(ubcollide PRIVATE libunabashed-O0 Threads::Threads)
  target_include_directories(ubcollide PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(checklib env NM=${CMAKE_NM} OBJDUMP=${CMAKE_OBJDUMP}
    sh ${CMAKE_CURRENT_SOURCE_DIR}/tools/check-lib
    $<TARGET_OBJECTS:libunabashed> $<TARGET_OBJECTS:libunabashed-O0>)
if(WIN32)
  # Built for Windows, perhaps elsewhere: the hosts are built, not run
  set(hosts ubhost)
  if(BITS32)
    list(APPEND hosts ubtiny)
  endif()
  add_custom_target(check-lib COMMAND ${checklib} DEPENDS ${hosts} VERBATIM)
else()
  add_custom_target(check-lib
    COMMAND ${checklib}
    COMMAND ubcollide
    COMMAND ubhost -c "echo $((6*7)) | grep -q 42"
    DEPENDS ubhost ubcollide VERBATIM)
endif()

add_custom_target(check COMMAND tests DEPENDS tests VERBATIM)

# What w64devkit's Dockerfile takes: the committed tree, named after the
# commit's date and hash
add_custom_target(dist
  COMMAND sh -c [[v=$(git log -1 --date=format:%Y%m%d --format=%cd-%h) &&
    git archive --prefix=unabashed-$v/ -o "$0/unabashed-$v.tar.gz" HEAD &&
    echo "$0/unabashed-$v.tar.gz"]] ${CMAKE_CURRENT_BINARY_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} VERBATIM)
