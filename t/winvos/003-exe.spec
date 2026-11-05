== stdout 264
plain -
tool.EXE x
run.bat x
x.sh x
y.com x
shebang x
mz -
C:/w64/bin/ls.exe x
cmdline = <.\mine a>
argv[0] = <.\mine>
argv[1] = <a>
cmdline = <.\mine.exe b>
argv[0] = <.\mine.exe>
argv[1] = <b>
cmdline = <mine c>
argv[0] = <mine>
argv[1] = <c>
script d
shebang e

== stderr 0

== merged 264
plain -
tool.EXE x
run.bat x
x.sh x
y.com x
shebang x
mz -
C:/w64/bin/ls.exe x
cmdline = <.\mine a>
argv[0] = <.\mine>
argv[1] = <a>
cmdline = <.\mine.exe b>
argv[0] = <.\mine.exe>
argv[1] = <b>
cmdline = <mine c>
argv[0] = <mine>
argv[1] = <c>
script d
shebang e

== status 0
