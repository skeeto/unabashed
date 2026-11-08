# Development conveniences. Every target is also a single compiler
# command; see the header of each main_*.cpp.
#
# Windows (w64devkit):  make unabashed.exe unabashed32.exe debug.exe tests.exe
#                       argv.exe argv32.exe (tools/argv.cpp, for t/msys)
# Linux:                make unabashed debug tests
# Fuzzing (AFL++):      make fuzz
# Source tarball:       make dist (HEAD as unabashed-YYYYMMDD-HASH.tar.gz)

CXX     = g++
CXX32   = i686-w64-mingw32-g++
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
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -Wl,--stack,0x2000000 -o $@ main_test.cpp

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
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -o $@ main_test.cpp

fuzz: main_fuzz.cpp $(SOURCES)
	$(AFLCXX) $(FLAGS) $(DEBUG) -o $@ main_fuzz.cpp

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
	      tests tests.exe fuzz argv.exe argv32.exe

.PHONY: default check clean
