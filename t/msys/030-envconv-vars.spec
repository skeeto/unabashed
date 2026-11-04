== stdout 260
cmdline = <@WORK@\bin\argv.exe>
argv[0] = <@WORK@\bin\argv.exe>
FOO=@WORK@\030-envconv-vars\tmp\a;@ROOT@\usr
BAR=C:/x
BAZ=http://x/y
QUX=C:\x
ONE=X:/
OPT=--dir=C:/x
LIST=/c/a;/c/b
REL=a/b
EMPTY=
TILDE=~/x
SHELL=@GIT@\usr\bin\bash.exe
LD_LIBRARY_PATH=C:\a;C:\b

== stderr 0

== merged 260
cmdline = <@WORK@\bin\argv.exe>
argv[0] = <@WORK@\bin\argv.exe>
FOO=@WORK@\030-envconv-vars\tmp\a;@ROOT@\usr
BAR=C:/x
BAZ=http://x/y
QUX=C:\x
ONE=X:/
OPT=--dir=C:/x
LIST=/c/a;/c/b
REL=a/b
EMPTY=
TILDE=~/x
SHELL=@GIT@\usr\bin\bash.exe
LD_LIBRARY_PATH=C:\a;C:\b

== status 0
