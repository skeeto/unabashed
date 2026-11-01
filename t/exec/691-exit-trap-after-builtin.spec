== stdout 73
status 0
xtrap
status 3
declare -- v="1"
status 1
after
in trap
status 0

== stderr 175
bash: line 1: T: command not found
bash: line 1: nosuch: command not found
bash: line 5: undefined: unbound variable
bash: line 1: cd: /nonexistent: No such file or directory

== merged 248
bash: line 1: T: command not found
status 0
xtrap
bash: line 1: nosuch: command not found
status 3
bash: line 5: undefined: unbound variable
declare -- v="1"
status 1
after
in trap
bash: line 1: cd: /nonexistent: No such file or directory
status 0

== status 0
