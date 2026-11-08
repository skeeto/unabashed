# personality: w64bash
# Hand-checked baseline (M9 step 3). w64devkit's bash.exe: msys mode in
# an install with no etc/fstab and no usr/bin. / is the install root,
# /bin the shell's own directory, drives are /c, and /tmp is the user's
# temporary directory (TEMP), as Git for Windows' fstab mounts it, so
# that /tmp paths work although the install has no tmp.
cygpath -w / /bin /tmp
cygpath -u C:/Users/oracle/AppData/Local/Temp C:/w64/bin C:/x
cd /tmp && pwd && pwd -W
echo hi >/tmp/f && cat C:/Users/oracle/AppData/Local/Temp/f
rm /tmp/f
cd /TMP && pwd -W
cd /
echo /*
