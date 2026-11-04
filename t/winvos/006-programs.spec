== stdout 420
2
applet
status 127
*.txt {x,y} @a.txt
a.txt
b.txt
cmdline = <C:/w64/bin/argv.exe here>
argv[0] = <C:/w64/bin/argv.exe>
argv[1] = <here>
C:/work/sub
cmdline = <argv>
argv[0] = <argv>
PATH=C:\w64\bin;C:\Windows\System32
Path=C:\w64\bin;C:\Windows\System32
TEMP=C:\Users\oracle\AppData\Local\Temp
nosuch unset
cmdline = <argv>
argv[0] = <argv>
fd 1 open
argv: fd 1
cmdline = <argv>
argv[0] = <argv>
fd 3 closed
status 127

== stderr 68
nosuch: applet not found
bash: line 23: printenv: command not found

== merged 488
2
applet
nosuch: applet not found
status 127
*.txt {x,y} @a.txt
a.txt
b.txt
cmdline = <C:/w64/bin/argv.exe here>
argv[0] = <C:/w64/bin/argv.exe>
argv[1] = <here>
C:/work/sub
cmdline = <argv>
argv[0] = <argv>
PATH=C:\w64\bin;C:\Windows\System32
Path=C:\w64\bin;C:\Windows\System32
TEMP=C:\Users\oracle\AppData\Local\Temp
nosuch unset
cmdline = <argv>
argv[0] = <argv>
fd 1 open
argv: fd 1
cmdline = <argv>
argv[0] = <argv>
fd 3 closed
bash: line 23: printenv: command not found
status 127

== status 0
