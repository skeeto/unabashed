# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a steps 4, 5).
# The working directory and cd with drive paths in every spelling. In
# native mode (busybox-w32's rules) PWD is the canonical native path,
# /c/Windows is C:/c/Windows on the current drive, and case is kept.
pwd
echo "$PWD"
cd /c/Windows && pwd
cd C:/Windows/System32 && pwd
cd 'C:\Windows' && pwd
cd c:/WORK && pwd
cd D: && pwd
cd E:/
echo "status $?"
cd .. && pwd
cd C:/work/nosuch
echo "status $?"
cd C:/work && mkdir sub && cd sub && pwd && cd .. && pwd
