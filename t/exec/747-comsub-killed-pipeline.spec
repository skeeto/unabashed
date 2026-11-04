== stdout 177
[]
status 0 0 0
status 0 0 0
status 1 0 1
status 143 0 143
status 0 0 137 0
[]
status 0 0 0
[]
status 0 0 0
[] 
status 0 0 0
[]
status 0
status 143
[]
status 0
status 143 0 143

== stderr 115
[]
bash: line 7: /nonexistent: No such file or directory
Terminated                 true | ( kill -TERM $BASHPID )

== merged 292
[]
status 0 0 0
[]
status 0 0 0
bash: line 7: /nonexistent: No such file or directory
status 1 0 1
status 143 0 143
status 0 0 137 0
[]
status 0 0 0
[]
status 0 0 0
[] 
status 0 0 0
[]
status 0
status 143
[]
status 0
Terminated                 true | ( kill -TERM $BASHPID )
status 143 0 143

== status 0
