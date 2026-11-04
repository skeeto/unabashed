# personality: msys
# Hand-checked baseline, not recorded from an oracle (M5a steps 4, 5);
# step 7 strips .exe from type's answers.
# The shell as C:/msys/usr/bin/bash.exe: the mount table of the
# install's etc/fstab, PWD and PATH in POSIX form.
pwd
echo "$PATH"
type bash ls
cd /
pwd
ls C:/msys/etc
cat /c/msys/etc/fstab
