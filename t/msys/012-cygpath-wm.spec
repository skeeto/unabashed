== stdout 543
/c -> C:\ C:/
/c/Windows -> C:\Windows C:/Windows
/C/Windows/ -> C:\Windows\ C:/Windows/
/tmp -> @WORK@\012-cygpath-wm\tmp @WORK@/012-cygpath-wm/tmp
/tmp/x -> @WORK@\012-cygpath-wm\tmp\x @WORK@/012-cygpath-wm/tmp/x
/usr/bin -> @GIT@\usr\bin @GIT@/usr/bin
/bin -> @GIT@\usr\bin @GIT@/usr/bin
/etc/fstab -> @GIT@\etc\fstab @GIT@/etc/fstab
/usr -> @ROOT@\usr @ROOT@/usr
/x -> X:\ X:/
/d/a/b -> D:\a\b D:/a/b
test.sh -> test.sh test.sh
a/b -> a\b a/b
./a -> a a
../a -> @WORK@\a @WORK@/a
. -> . .
/dev/null -> \\.\NUL //./NUL
/proc -> /proc /proc

== stderr 0

== merged 543
/c -> C:\ C:/
/c/Windows -> C:\Windows C:/Windows
/C/Windows/ -> C:\Windows\ C:/Windows/
/tmp -> @WORK@\012-cygpath-wm\tmp @WORK@/012-cygpath-wm/tmp
/tmp/x -> @WORK@\012-cygpath-wm\tmp\x @WORK@/012-cygpath-wm/tmp/x
/usr/bin -> @GIT@\usr\bin @GIT@/usr/bin
/bin -> @GIT@\usr\bin @GIT@/usr/bin
/etc/fstab -> @GIT@\etc\fstab @GIT@/etc/fstab
/usr -> @ROOT@\usr @ROOT@/usr
/x -> X:\ X:/
/d/a/b -> D:\a\b D:/a/b
test.sh -> test.sh test.sh
a/b -> a\b a/b
./a -> a a
../a -> @WORK@\a @WORK@/a
. -> . .
/dev/null -> \\.\NUL //./NUL
/proc -> /proc /proc

== status 0
