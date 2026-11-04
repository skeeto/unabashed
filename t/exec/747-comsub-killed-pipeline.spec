== stdout 334
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
status 3 0 3
trap
status 143 0 143
status 0 0 3 0
status 3
status 143
status 143
status 143
status 143
err
status 143
debug
status 143
status 143
status 143

== stderr 719
[]
bash: line 9: /nonexistent: No such file or directory
Terminated                 true | ( kill -TERM $BASHPID )
Terminated                 bash -c 'kill $$'
Terminated                 bash -c 'kill $$'
Terminated                 bash -c 'kill $$'
Terminated                 bash -c 'kill $$'
Terminated                 bash -c 'kill $$'
Terminated                 ( trap - EXIT; bash -c 'kill $$' )
Terminated                 bash -c 'kill $$'
Terminated                 ( trap '' USR1; bash -c 'kill $$' )
Terminated                 bash -c 'kill $$'
Terminated                 ( trap 'echo debug' DEBUG; bash -c 'kill $$' )
Terminated                 bash -c 'kill $$'
Terminated                 bash -c 'kill $$'

== merged 1053
[]
status 0 0 0
[]
status 0 0 0
bash: line 9: /nonexistent: No such file or directory
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
Terminated                 bash -c 'kill $$'
status 3 0 3
Terminated                 bash -c 'kill $$'
trap
status 143 0 143
Terminated                 bash -c 'kill $$'
status 0 0 3 0
Terminated                 bash -c 'kill $$'
status 3
Terminated                 bash -c 'kill $$'
status 143
Terminated                 ( trap - EXIT; bash -c 'kill $$' )
status 143
Terminated                 bash -c 'kill $$'
status 143
Terminated                 ( trap '' USR1; bash -c 'kill $$' )
status 143
Terminated                 bash -c 'kill $$'
err
status 143
debug
Terminated                 ( trap 'echo debug' DEBUG; bash -c 'kill $$' )
status 143
Terminated                 bash -c 'kill $$'
status 143
Terminated                 bash -c 'kill $$'
status 143

== status 0
