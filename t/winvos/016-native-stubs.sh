# personality: native
# Hand-checked baseline, not recorded from an oracle (M5a step 7).
# A w64devkit alias stub (a small CRT-free image beside busybox.exe,
# naming it in a message) is bypassed: busybox.exe runs directly with the
# applet as argv[0], unless BB_OVERRIDE_APPLETS names the applet ("-"
# for all). Other
# small CRT-free programs there run as themselves (argvk.exe, as
# w64devkit's u-config and its cc.exe alias for gcc). argv.exe
# stands in for busybox.exe to show what it gets. Also: busybox's
# libtool hack (cmd //c echo), and a command line too long for
# CreateProcess fails with status 126.
mkdir s
cp C:/w64/bin/ls.exe s/ls.exe
cp C:/w64/bin/argv.exe s/busybox.exe
s/ls -d 'a b'
BB_OVERRIDE_APPLETS='x,ls' s/ls.exe 1
BB_OVERRIDE_APPLETS=- s/ls 2
BB_OVERRIDE_APPLETS=+ls s/ls 3
BB_OVERRIDE_APPLETS=lsx s/ls 4
cp C:/w64/bin/argvk.exe s/pkg-config.exe
s/pkg-config 5
cmd //c echo hi
cmd /c echo hi
x=$(printf '%040000d' 0)
argv $x $x $x $x $x $x $x $x $x $x; echo "status $?"
