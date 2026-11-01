== stdout 372
[1]-  Running                    sleep 1 &
[3]+  Running                    sleep 3 &
[1]-  Running                    sleep 1 &
[3]+  Running                    sleep 3 &
[1]+  Running                    sleep 1 &
s 1
s 1
s 2
none 1
[1]   Exit 1                     { sleep 1; exit 1; }
w 0
fg 1
bg 1
suspend 1
suspend 2
kill %1 143
one left 0
no job 1
ambiguous 1
end 0

== stderr 502
bash: line 9: disown: %5: no such job
bash: line 10: disown: 12345: no such job
bash: line 11: disown: -z: invalid option
disown: usage: disown [-h] [-ar] [jobspec ... | pid ...]
bash: line 15: disown: current: no such job
bash: line 21: fg: no job control
bash: line 22: bg: no job control
bash: line 23: suspend: cannot suspend: no job control
bash: line 24: suspend: -z: invalid option
suspend: usage: suspend [-f]
bash: line 28: kill: %9: no such job
bash: line 30: kill: sleep: ambiguous job spec

== merged 874
[1]-  Running                    sleep 1 &
[3]+  Running                    sleep 3 &
[1]-  Running                    sleep 1 &
[3]+  Running                    sleep 3 &
[1]+  Running                    sleep 1 &
bash: line 9: disown: %5: no such job
s 1
bash: line 10: disown: 12345: no such job
s 1
bash: line 11: disown: -z: invalid option
disown: usage: disown [-h] [-ar] [jobspec ... | pid ...]
s 2
bash: line 15: disown: current: no such job
none 1
[1]   Exit 1                     { sleep 1; exit 1; }
w 0
bash: line 21: fg: no job control
fg 1
bash: line 22: bg: no job control
bg 1
bash: line 23: suspend: cannot suspend: no job control
suspend 1
bash: line 24: suspend: -z: invalid option
suspend: usage: suspend [-f]
suspend 2
kill %1 143
one left 0
bash: line 28: kill: %9: no such job
no job 1
bash: line 30: kill: sleep: ambiguous job spec
ambiguous 1
end 0

== status 0
