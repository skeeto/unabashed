== stdout 201
argv is @WORK@/bin/argv
argv32 is @WORK@/bin/argv32
@WORK@/bin/argv
@WORK@/bin/argv.exe
cc is @W64@/bin/cc
/c/WINDOWS/System32/cmd
/c/WINDOWS/System32/CMD
/c/WINDOWS/System32/cmd.exe
status 1
status 1

== stderr 89
test.sh: line 10: hash: argv: not found
test.sh: line 11: type: nosuchprogram: not found

== merged 290
argv is @WORK@/bin/argv
argv32 is @WORK@/bin/argv32
@WORK@/bin/argv
@WORK@/bin/argv.exe
cc is @W64@/bin/cc
/c/WINDOWS/System32/cmd
/c/WINDOWS/System32/CMD
/c/WINDOWS/System32/cmd.exe
test.sh: line 10: hash: argv: not found
test.sh: line 11: type: nosuchprogram: not found
status 1
status 1

== status 0
