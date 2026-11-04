== stdout 558
cmdline = <@WORK@\bin\argv.exe /c/y>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = </c/y>
FOO=/c/x
HOME=@WORK@\032-envconv-nopathconv\home
TEMP=@WORK@\032-envconv-nopathconv\tmp
PATH=@GIT@\usr\bin;@W64@\bin;C:\WINDOWS\System32;@WORK@\bin
MSYS_NO_PATHCONV=1
MSYS2_ARG_CONV_EXCL unset
cmdline = <@WORK@\bin\argv.exe /c/y>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = </c/y>
FOO=C:/x
HOME=@WORK@\032-envconv-nopathconv\home
TEMP=@WORK@\032-envconv-nopathconv\tmp
PATH=@GIT@\usr\bin;@W64@\bin;C:\WINDOWS\System32;@WORK@\bin
MSYS_NO_PATHCONV unset
MSYS2_ARG_CONV_EXCL=*

== stderr 0

== merged 558
cmdline = <@WORK@\bin\argv.exe /c/y>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = </c/y>
FOO=/c/x
HOME=@WORK@\032-envconv-nopathconv\home
TEMP=@WORK@\032-envconv-nopathconv\tmp
PATH=@GIT@\usr\bin;@W64@\bin;C:\WINDOWS\System32;@WORK@\bin
MSYS_NO_PATHCONV=1
MSYS2_ARG_CONV_EXCL unset
cmdline = <@WORK@\bin\argv.exe /c/y>
argv[0] = <@WORK@\bin\argv.exe>
argv[1] = </c/y>
FOO=C:/x
HOME=@WORK@\032-envconv-nopathconv\home
TEMP=@WORK@\032-envconv-nopathconv\tmp
PATH=@GIT@\usr\bin;@W64@\bin;C:\WINDOWS\System32;@WORK@\bin
MSYS_NO_PATHCONV unset
MSYS2_ARG_CONV_EXCL=*

== status 0
