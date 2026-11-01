# Development conveniences. Every target is also a single compiler
# command; see the header of each main_*.cpp.
#
# Windows (w64devkit):  make unabashed.exe unabashed32.exe debug.exe tests.exe
# Linux:                make unabashed debug tests
# Fuzzing (AFL++):      make fuzz

CXX     = g++
CXX32   = i686-w64-mingw32-g++
AFLCXX  = afl-clang-fast++
FLAGS   = -std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics \
          -fno-asynchronous-unwind-tables
WARN    = -Wall -Wextra -Wconversion -Wno-sign-conversion \
          -Wno-unused-parameter -Wno-unused-function
DEBUG   = -g3 -fsanitize=undefined -fsanitize-trap
RELEASE = -O2 -s
WIN32   = -nostartfiles -static-libgcc -Wl,--stack,0x1000000
SOURCES = base.cpp api.cpp unabashed.cpp parse.cpp print.cpp wide.cpp \
          fibers.cpp check.cpp shell.cpp array.cpp vars.cpp expand.cpp exec.cpp \
          builtins.cpp arith.cpp pattern.cpp brace.cpp \
          b_test.cpp b_printf.cpp vos.cpp fakes.cpp

default:
	@echo "pick a target, e.g. make debug.exe or make tests" && false

unabashed.exe: main_win32.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(RELEASE) $(WIN32) -o $@ main_win32.cpp

unabashed32.exe: main_win32.cpp $(SOURCES)
	$(CXX32) $(FLAGS) $(WARN) $(RELEASE) $(WIN32) \
	    -Wl,--major-subsystem-version=5,--minor-subsystem-version=1 \
	    -o $@ main_win32.cpp

debug.exe: main_win32.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) $(WIN32) -o $@ main_win32.cpp

tests.exe: main_test.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -Wl,--stack,0x2000000 -o $@ main_test.cpp

unabashed: main_posix.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(RELEASE) -o $@ main_posix.cpp

debug: main_posix.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -o $@ main_posix.cpp

tests: main_test.cpp $(SOURCES)
	$(CXX) $(FLAGS) $(WARN) $(DEBUG) -o $@ main_test.cpp

fuzz: main_fuzz.cpp $(SOURCES)
	$(AFLCXX) $(FLAGS) $(DEBUG) -o $@ main_fuzz.cpp

check: tests
	./tests

clean:
	rm -f unabashed unabashed.exe unabashed32.exe debug debug.exe \
	      tests tests.exe fuzz

.PHONY: default check clean
