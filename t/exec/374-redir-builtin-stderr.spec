== stdout 287
status 1
[bash: line 3: cd: /nonexistent: No such file or directory]
status 1
[bash: line 3: cd: /nonexistent: No such file or directory]
inside
[bash: line 9: cd: /nonexistent: No such file or directory]
status 127
[bash: line 11: nosuchcmd_xyz: command not found]
status 1
e4 status 1

== stderr 105
bash: line 14: /tmp/nodir/x: No such file or directory
bash: line 16: /tmp/e4: No such file or directory

== merged 392
status 1
[bash: line 3: cd: /nonexistent: No such file or directory]
status 1
[bash: line 3: cd: /nonexistent: No such file or directory]
inside
[bash: line 9: cd: /nonexistent: No such file or directory]
status 127
[bash: line 11: nosuchcmd_xyz: command not found]
bash: line 14: /tmp/nodir/x: No such file or directory
status 1
bash: line 16: /tmp/e4: No such file or directory
e4 status 1

== status 0
