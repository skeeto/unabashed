== stdout 22
subshell status 2
i=1

== stderr 102
bash: line 2: break: x: numeric argument required
bash: line 6: break: abc: numeric argument required

== merged 124
bash: line 2: break: x: numeric argument required
subshell status 2
i=1
bash: line 6: break: abc: numeric argument required

== status 2
