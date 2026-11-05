== stdout 832
a b *  q"q {x,y} @f ~ /c/x --d=/tmp a\b
cmdline = <C:\w64\bin\argv.exe "a b" * C:/x --d=C:/Users/oracle/AppData/Local/Temp a:/b>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <a b>
argv[2] = <*>
argv[3] = <C:/x>
argv[4] = <--d=C:/Users/oracle/AppData/Local/Temp>
argv[5] = <a:/b>
cmdline = <C:\w64\bin\argv.exe --d=/tmp C:/Users/oracle/AppData/Local/Temp>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <--d=/tmp>
argv[2] = <C:/Users/oracle/AppData/Local/Temp>
cmdline = <C:\w64\bin\argv.exe>
argv[0] = <C:\w64\bin\argv.exe>
FOO=C:/x
BAR=C:/Users/oracle/AppData/Local/Temp
SHELL=C:\msys\usr\bin\bash.exe
SYSTEMROOT=C:\Windows
WINDIR=C:\Windows
cmdline = <C:\w64\bin\argv.exe>
argv[0] = <C:\w64\bin\argv.exe>
FOO=/c/x
BAR=C:/Users/oracle/AppData/Local/Temp
BAR=/tmp
FOO=/c/x
SHELL=C:\msys\usr\bin\bash.exe
[C:/x] ["a b"] ["%PATH%"]
status 0

== stderr 0

== merged 832
a b *  q"q {x,y} @f ~ /c/x --d=/tmp a\b
cmdline = <C:\w64\bin\argv.exe "a b" * C:/x --d=C:/Users/oracle/AppData/Local/Temp a:/b>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <a b>
argv[2] = <*>
argv[3] = <C:/x>
argv[4] = <--d=C:/Users/oracle/AppData/Local/Temp>
argv[5] = <a:/b>
cmdline = <C:\w64\bin\argv.exe --d=/tmp C:/Users/oracle/AppData/Local/Temp>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <--d=/tmp>
argv[2] = <C:/Users/oracle/AppData/Local/Temp>
cmdline = <C:\w64\bin\argv.exe>
argv[0] = <C:\w64\bin\argv.exe>
FOO=C:/x
BAR=C:/Users/oracle/AppData/Local/Temp
SHELL=C:\msys\usr\bin\bash.exe
SYSTEMROOT=C:\Windows
WINDIR=C:\Windows
cmdline = <C:\w64\bin\argv.exe>
argv[0] = <C:\w64\bin\argv.exe>
FOO=/c/x
BAR=C:/Users/oracle/AppData/Local/Temp
BAR=/tmp
FOO=/c/x
SHELL=C:\msys\usr\bin\bash.exe
[C:/x] ["a b"] ["%PATH%"]
status 0

== status 0
