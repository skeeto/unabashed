# personality: msys
# Hand-checked against Git Bash 5.3.15 (M5a step 10a): /proc lists its
# fixed entries (the files and directories Cygwin has) and the shell's
# own pid, /proc/self the files the shell serves. The fixed files have
# no contents here (Git Bash's do).
echo /proc/s*
echo /proc/[a-m]*
(cd /proc && echo [n-v]*)
[ -f /proc/stat ] && echo stat is a file
[ -d /proc/sys ] && echo sys is a directory
[ -d /proc/cygdrive ] && echo cygdrive is a directory
[ -e /proc/nosuch ] || echo no nosuch
[ -d /proc/$$ ] && echo own pid is a directory
for d in /proc/[0-9]*; do [ "$d" = /proc/$$ ] && echo own pid listed; done
echo /proc/self/*
echo /proc/[a-z]*/
read -r x </proc/stat || echo stat has no contents
