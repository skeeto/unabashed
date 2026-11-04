# personality: msys
# Hand-checked baseline of today's minimal Windows layer, not recorded
# from an oracle (M5a step 4); step 5 gives the msys persona its mount
# table and POSIX paths, changing this on purpose.
# The shell as C:/msys/usr/bin/bash.exe: the same native paths as sh.
pwd
echo "$PATH"
type bash ls
cd /
pwd
ls C:/msys/etc
cat /c/msys/etc/fstab
