# personality: native
# Hand-checked baseline, not recorded from an oracle, checked against
# busybox-w32's win32/mingw.c. A native shell exports BB_GLOBBING=0, as
# busybox-w32's startup does, so that its programs know that a shell
# already expanded their arguments; an inherited value is kept as it is.
declare -p BB_GLOBBING
sh -c 'declare -p BB_GLOBBING'
BB_GLOBBING=1 sh -c 'declare -p BB_GLOBBING'
(unset BB_GLOBBING; sh -c 'declare -p BB_GLOBBING')
