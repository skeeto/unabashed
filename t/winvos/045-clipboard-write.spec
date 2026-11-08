== stdout 832
echo [hi
] 1
group [ab] 3
no write [ab] 3
append [x
] 4
exec 3> [one
two
] 6
program [cmdline = <C:\w64\bin\argv.exe p q>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <p>
argv[2] = <q>
] 7
silent program [cmdline = <C:\w64\bin\argv.exe p q>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <p>
argv[2] = <q>
] 7
mixed [a
cmdline = <C:\w64\bin\argv.exe b>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <b>
c
] 10
last stage [in
] 11
pipeline in group [s
t
] 13
subshell [u
cmdline = <C:\w64\bin\argv.exe v>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <v>
] 15
background [cmdline = <C:\w64\bin\argv.exe bg>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <bg>
] 16
background vproc [w
y
] 18
noclobber 0
noclobber [n
] 19
writable
fd 4 a device
0000000   c   a   f 303 251
0000005
0000000   a
0000001
busy 1
busy [a] 21
busy program 0
busy program [a] 21

== stderr 101
bash: line 32: echo: write error: Permission denied
bash: line 34: /dev/clipboard: Permission denied

== merged 933
echo [hi
] 1
group [ab] 3
no write [ab] 3
append [x
] 4
exec 3> [one
two
] 6
program [cmdline = <C:\w64\bin\argv.exe p q>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <p>
argv[2] = <q>
] 7
silent program [cmdline = <C:\w64\bin\argv.exe p q>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <p>
argv[2] = <q>
] 7
mixed [a
cmdline = <C:\w64\bin\argv.exe b>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <b>
c
] 10
last stage [in
] 11
pipeline in group [s
t
] 13
subshell [u
cmdline = <C:\w64\bin\argv.exe v>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <v>
] 15
background [cmdline = <C:\w64\bin\argv.exe bg>
argv[0] = <C:\w64\bin\argv.exe>
argv[1] = <bg>
] 16
background vproc [w
y
] 18
noclobber 0
noclobber [n
] 19
writable
fd 4 a device
0000000   c   a   f 303 251
0000005
0000000   a
0000001
bash: line 32: echo: write error: Permission denied
busy 1
busy [a] 21
bash: line 34: /dev/clipboard: Permission denied
busy program 0
busy program [a] 21

== status 0
