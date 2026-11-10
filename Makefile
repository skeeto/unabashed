# Development conveniences. Every target is also a single compiler
# command; see the header of each main_*.cpp.
#
# Windows (w64devkit):  make unabashed.exe unabashed32.exe debug.exe tests.exe
#                       argv.exe argv32.exe (tools/argv.cpp, for t/msys)
# Linux:                make unabashed debug tests
# Library (POSIX):      make libunabashed.o ubhost check-lib
# Library (Windows):    make libunabashed.obj ubhost.exe check-lib-win
#                       ubhost32.exe ubtiny.exe (32-bit, XP)
# Fuzzing (AFL++):      make fuzz
# Source tarball:       make dist (HEAD as unabashed-YYYYMMDD-HASH.tar.gz)

CXX     = g++
CC      = cc
CXX32   = i686-w64-mingw32-g++
CC32    = i686-w64-mingw32-gcc
AFLCXX  = afl-clang-fast++
FLAGS   = -std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
          -fno-asynchronous-unwind-tables
WARN    = -Wall -Wextra -Wconversion -Wno-sign-conversion \
          -Wno-unused-parameter -Wno-unused-function
DEBUG   = -g3 -fsanitize=undefined -fsanitize-trap
RELEASE = -O2 -s
WIN32   = -nostartfiles -static-libgcc
SOURCES = $(wildcard src/*.cpp)

default:
	@echo "pick a target, e.g. make debug.exe or make tests" && false

unabashed.exe: main_win32.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(RELEASE) $(WIN32) -o $@ main_win32.cpp

unabashed32.exe: main_win32.cpp $(SOURCES)
	$(CXX32) $(FLAGS) $(WARN) $(RELEASE) $(WIN32) \
	    -Wl,--major-subsystem-version=5,--minor-subsystem-version=1 \
	    -Wl,--large-address-aware -o $@ main_win32.cpp

debug.exe: main_win32.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) $(WIN32) -o $@ main_win32.cpp

tests.exe: main_test.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -Og -Wl,--stack,0x2000000 -o $@ main_test.cpp

# tools/argv.cpp keeps the CRT startup on purpose: it shows how msvcrt.dll
# splits a command line (tools/README.md).
argv.exe: tools/argv.cpp
	$(CXX) -std=c++20 -municode $(WARN) $(RELEASE) -o $@ tools/argv.cpp

argv32.exe: tools/argv.cpp
	$(CXX32) -std=c++20 -municode $(WARN) $(RELEASE) -o $@ tools/argv.cpp

unabashed: main_posix.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(RELEASE) -nostdlib++ -o $@ main_posix.cpp

debug: main_posix.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -o $@ main_posix.cpp

tests: main_test.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -Og -o $@ main_test.cpp

fuzz: main_fuzz.cpp $(SOURCES)
	$(AFLCXX) $(FLAGS) $(DEBUG) -o $@ main_fuzz.cpp

libunabashed.o: libunabashed.cpp unabashed.h $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) -O2 -c -o $@ libunabashed.cpp

libunabashed-O0.o: libunabashed.cpp unabashed.h $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) -O0 -c -o $@ libunabashed.cpp

ubhost: tools/ubhost.c tools/ubcheck.c unabashed.h libunabashed.o
	$(CC) -O2 -Wall -Wextra -Wno-missing-field-initializers \
	    -o $@ tools/ubhost.c libunabashed.o -lpthread

libunabashed.obj: libunabashed.cpp unabashed.h $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) -O2 -c -o $@ libunabashed.cpp

libunabashed-O0.obj: libunabashed.cpp unabashed.h $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) -O0 -c -o $@ libunabashed.cpp

# No C runtime: ubhostw.c brings the mem* and strlen the library needs
ubhost.exe: tools/ubhost.c tools/ubhostw.c tools/ubcheckw.c unabashed.h libunabashed.obj
	$(CC) -O2 -Wall -Wextra -Wno-missing-field-initializers -nostartfiles -nostdlib \
	    -o $@ tools/ubhost.c libunabashed.obj -lkernel32 -lgcc

libunabashed32.obj: libunabashed.cpp unabashed.h $(SOURCES)
	$(CXX32) $(FLAGS) $(WARN) -O2 -c -o $@ libunabashed.cpp

# 32-bit hosts load on XP: subsystem 5.1, kernel32 only
ubhost32.exe: tools/ubhost.c tools/ubhostw.c tools/ubcheckw.c unabashed.h libunabashed32.obj
	$(CC32) -O2 -Wall -Wextra -Wno-missing-field-initializers -nostartfiles -nostdlib \
	    -Wl,--major-subsystem-version=5,--minor-subsystem-version=1 \
	    -Wl,--large-address-aware -o $@ tools/ubhost.c libunabashed32.obj -lkernel32 -lgcc

ubtiny.exe: tools/ubtiny.c unabashed.h libunabashed32.obj
	$(CC32) -O2 -Wall -Wextra -nostartfiles -nostdlib \
	    -Wl,--major-subsystem-version=5,--minor-subsystem-version=1 \
	    -o $@ tools/ubtiny.c libunabashed32.obj -lkernel32 -lgcc

ubcollide: tools/ubcollide.cpp unabashed.h libunabashed-O0.o
	$(CXX) $(FLAGS) -nostdlib++ -o $@ tools/ubcollide.cpp libunabashed-O0.o -lpthread

check-lib: libunabashed.o libunabashed-O0.o ubhost ubcollide
	tools/check-lib libunabashed.o libunabashed-O0.o
	./ubcollide
	./ubhost -c 'echo $$((6*7)) | grep -q 42'

check-lib-win: libunabashed.obj libunabashed-O0.obj libunabashed32.obj ubhost.exe \
               ubhost32.exe ubtiny.exe
	tools/check-lib libunabashed.obj libunabashed-O0.obj libunabashed32.obj

# What w64devkit's Dockerfile takes: the committed tree, named after the
# commit's date and hash.
dist:
	v=$$(git log -1 --date=format:%Y%m%d --format=%cd-%h) && \
	git archive --prefix=unabashed-$$v/ -o unabashed-$$v.tar.gz HEAD && \
	echo unabashed-$$v.tar.gz

check: tests
	./tests

clean:
	rm -f unabashed unabashed.exe unabashed32.exe debug debug.exe \
	      tests tests.exe fuzz argv.exe argv32.exe \
	      libunabashed.o libunabashed-O0.o ubhost ubcollide \
	      libunabashed.obj libunabashed-O0.obj ubhost.exe \
	      libunabashed32.obj ubhost32.exe ubtiny.exe

.PHONY: default check check-lib check-lib-win clean
