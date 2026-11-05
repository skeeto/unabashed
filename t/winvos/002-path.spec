== stdout 511
C:/w64/bin;C:/Windows/System32
argv is C:/w64/bin/argv.exe
sh is C:/w64/bin/sh.exe
cmd is C:/Windows/System32/cmd.exe
busybox is C:/w64/bin/busybox.exe
C:/w64/bin/ls.exe
C:/w64/bin/cat.exe
cmdline = <argv a "b c" "" "d\"e" f\ "*">
argv[0] = <argv>
argv[1] = <a>
argv[2] = <b c>
argv[3] = <>
argv[4] = <d"e>
argv[5] = <f\>
argv[6] = <*>
status 0
cmdline = <argv32 x>
argv[0] = <argv32>
argv[1] = <x>
cmdline = <C:\w64\bin\argv.exe y>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <y>
status 127
C:\Windows\System32

== stderr 136
bash: line 15: /c/w64/bin/argv: No such file or directory
bash: line 16: nosuch: command not found
bash: line 19: type: argv: not found

== merged 647
C:/w64/bin;C:/Windows/System32
argv is C:/w64/bin/argv.exe
sh is C:/w64/bin/sh.exe
cmd is C:/Windows/System32/cmd.exe
busybox is C:/w64/bin/busybox.exe
C:/w64/bin/ls.exe
C:/w64/bin/cat.exe
cmdline = <argv a "b c" "" "d\"e" f\ "*">
argv[0] = <argv>
argv[1] = <a>
argv[2] = <b c>
argv[3] = <>
argv[4] = <d"e>
argv[5] = <f\>
argv[6] = <*>
status 0
cmdline = <argv32 x>
argv[0] = <argv32>
argv[1] = <x>
cmdline = <C:\w64\bin\argv.exe y>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <y>
bash: line 15: /c/w64/bin/argv: No such file or directory
bash: line 16: nosuch: command not found
status 127
bash: line 19: type: argv: not found
C:\Windows\System32

== status 0
