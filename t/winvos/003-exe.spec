== stdout 210
plain -
tool.EXE x
run.bat x
x.sh x
y.com x
shebang x
mz x
C:/w64/bin/ls.exe x
cmdline = <./mine.exe b>
argv[0] = <./mine.exe>
argv[1] = <b>
cmdline = <mine c>
argv[0] = <mine>
argv[1] = <c>
script d
shebang e

== stderr 49
bash: line 17: ./mine: No such file or directory

== merged 259
plain -
tool.EXE x
run.bat x
x.sh x
y.com x
shebang x
mz x
C:/w64/bin/ls.exe x
bash: line 17: ./mine: No such file or directory
cmdline = <./mine.exe b>
argv[0] = <./mine.exe>
argv[1] = <b>
cmdline = <mine c>
argv[0] = <mine>
argv[1] = <c>
script d
shebang e

== status 0
