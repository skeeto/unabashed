== stdout 31
line 1
line 3
back 11
f line 5

== stderr 148
/tmp/ln.sh: line 4: nosuchcmd_abc: command not found
bash: line 13: nosuchcmd_def: command not found
/tmp/ln2.sh: line 1: undefined_v: message here

== merged 179
line 1
line 3
/tmp/ln.sh: line 4: nosuchcmd_abc: command not found
back 11
f line 5
bash: line 13: nosuchcmd_def: command not found
/tmp/ln2.sh: line 1: undefined_v: message here

== status 127
