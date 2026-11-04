# personality: native
# Hand-checked baseline of today's minimal Windows layer, not recorded
# from an oracle (M5a step 4); step 7 changes it on purpose.
# busybox applets through alias stubs and by name, MSYS2 programs given
# argv (no Cygwin globbing without a command line from the core), the
# environment and working directory children see.
echo one two | wc -w
busybox echo applet
busybox nosuch
echo "status $?"
: >a.txt
: >b.txt
C:/msys/usr/bin/echo.exe '*.txt' "{x,y}" '@a.txt'
/c/msys/usr/bin/ls.exe
mkdir sub
cd sub
C:/w64/bin/argv.exe here
pwd
cd ..
ARGV_SHOW='PATH Path TEMP nosuch' argv
ARGV_FD=1 argv
ARGV_FD=3 argv 3>&1
printenv HOME
echo "status $?"
