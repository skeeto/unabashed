== stdout 3605
st=147
[1]+  Stopped                    bash -c "kill -STOP \$\$"
[1]+ PID Stopped (signal)           bash -c "kill -STOP \$\$"
[]
bash -c "kill -STOP \$\$"
fgst=0
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
bash: line 1: warning: deleting stopped job 1 with process group N
st=0
st=0
bash: line 1: warning: deleting stopped job 1 with process group N
st=0
[1]+  Running                    sleep 0.3 > /dev/null 2>&1 &
same
[1]+ sleep 0.3 > /dev/null 2>&1 &
[1]+  Running                    sleep 0.3 > /dev/null 2>&1 &
bash: line 1: bg: job 1 already in background
st=0
[1]+  Running                    sleep 1 > /dev/null 2>&1 &
[1]+ PID Stopped                    sleep 1 > /dev/null 2>&1
[1]+ PID Stopped                    sleep 1 > /dev/null 2>&1
[1]+  Killed                     sleep 1 > /dev/null 2>&1
bash: line 1: warning: wait_for_job: job 1 is stopped
st=147
st=127
bash: line 1: wait: warning: job 1[PID] stopped
st=0
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
st=147
[1]   Running                    sleep 1 > /dev/null 2>&1 &
[2]+  Stopped                    sleep 1 > /dev/null 2>&1
[3]-  Running                    sleep 1 > /dev/null 2>&1 &
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
[2]-  Stopped                    sleep 1 > /dev/null 2>&1
[3]   Running                    sleep 1 > /dev/null 2>&1 &
[1]+  Running                    sleep 1 > /dev/null 2>&1 &
[2]-  Running                    sleep 1 > /dev/null 2>&1 &
[3]   Running                    sleep 1 > /dev/null 2>&1 &
sleep 0.3 > /dev/null 2>&1
st=0
sleep 0.3 > /dev/null 2>&1
st=0
bash: line 1: suspend: -x: invalid option
suspend: usage: suspend [-f]
2
bash: line 1: suspend: cannot suspend: no job control
1
in f 147
[1]-  Stopped                    bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -TSTP \$\$"
[1]- PID Stopped (signal)           bash -c "kill -STOP \$\$"
[2]+ PID Stopped                    bash -c "kill -TSTP \$\$"
1 147
2 147
after 0
[1]-  Stopped                    bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -STOP \$\$"
after 148
148
inbrace 148
or
in f 148
after 0
1 149
2 149
next
x
[1]+  Done                       sleep 0.2 > /dev/null 2>&1
[1]+  Killed                     sleep 1 > /dev/null 2>&1
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
[1]+  Killed                     sleep 1 > /dev/null 2>&1
[1]+  Stopped                    x=1 bash -c "kill -STOP \$\$" > /dev/null 2>&1
[1]-  Stopped                    bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -STOP \$\$"
bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -STOP \$\$"
st=3
got TSTP
after
after
149
[1]+  Stopped                    bash -c "kill -TTIN \$\$"
bash -c "kill -TTIN \$\$"
bash -c "kill -STOP \$\$"
st=0
bash -c "kill -STOP \$\$; exit 4"
st=4
st=147
bash -c "kill -STOP \$\$; kill -STOP \$\$; exit 5"
st=147
[1]+  Stopped                    bash -c "kill -STOP \$\$; kill -STOP \$\$; exit 5"
bash -c "kill -STOP \$\$; kill -STOP \$\$; exit 5"
st=5
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
-
-
-
[1]+  Running                    sleep 1 > /dev/null 2>&1 &
[1]?  Terminated                 sleep 1 > /dev/null 2>&1
bash: line 1: warning: wait_for_job: job 1 is stopped
st=0
[2]   Done                       ( sleep 0.3; kill -CONT $p )
st=0
[2]   Done                       ( sleep 0.2; kill -STOP $p; sleep 0.3; kill -CONT $p )
st=0
[2]   Done                       ( sleep 0.2; kill -STOP $p; sleep 0.3; kill -CONT $p ) > /dev/null 2>&1

== stderr 0

== merged 3605
st=147
[1]+  Stopped                    bash -c "kill -STOP \$\$"
[1]+ PID Stopped (signal)           bash -c "kill -STOP \$\$"
[]
bash -c "kill -STOP \$\$"
fgst=0
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
bash: line 1: warning: deleting stopped job 1 with process group N
st=0
st=0
bash: line 1: warning: deleting stopped job 1 with process group N
st=0
[1]+  Running                    sleep 0.3 > /dev/null 2>&1 &
same
[1]+ sleep 0.3 > /dev/null 2>&1 &
[1]+  Running                    sleep 0.3 > /dev/null 2>&1 &
bash: line 1: bg: job 1 already in background
st=0
[1]+  Running                    sleep 1 > /dev/null 2>&1 &
[1]+ PID Stopped                    sleep 1 > /dev/null 2>&1
[1]+ PID Stopped                    sleep 1 > /dev/null 2>&1
[1]+  Killed                     sleep 1 > /dev/null 2>&1
bash: line 1: warning: wait_for_job: job 1 is stopped
st=147
st=127
bash: line 1: wait: warning: job 1[PID] stopped
st=0
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
st=147
[1]   Running                    sleep 1 > /dev/null 2>&1 &
[2]+  Stopped                    sleep 1 > /dev/null 2>&1
[3]-  Running                    sleep 1 > /dev/null 2>&1 &
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
[2]-  Stopped                    sleep 1 > /dev/null 2>&1
[3]   Running                    sleep 1 > /dev/null 2>&1 &
[1]+  Running                    sleep 1 > /dev/null 2>&1 &
[2]-  Running                    sleep 1 > /dev/null 2>&1 &
[3]   Running                    sleep 1 > /dev/null 2>&1 &
sleep 0.3 > /dev/null 2>&1
st=0
sleep 0.3 > /dev/null 2>&1
st=0
bash: line 1: suspend: -x: invalid option
suspend: usage: suspend [-f]
2
bash: line 1: suspend: cannot suspend: no job control
1
in f 147
[1]-  Stopped                    bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -TSTP \$\$"
[1]- PID Stopped (signal)           bash -c "kill -STOP \$\$"
[2]+ PID Stopped                    bash -c "kill -TSTP \$\$"
1 147
2 147
after 0
[1]-  Stopped                    bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -STOP \$\$"
after 148
148
inbrace 148
or
in f 148
after 0
1 149
2 149
next
x
[1]+  Done                       sleep 0.2 > /dev/null 2>&1
[1]+  Killed                     sleep 1 > /dev/null 2>&1
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
[1]+  Killed                     sleep 1 > /dev/null 2>&1
[1]+  Stopped                    x=1 bash -c "kill -STOP \$\$" > /dev/null 2>&1
[1]-  Stopped                    bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -STOP \$\$"
bash -c "kill -STOP \$\$"
[2]+  Stopped                    bash -c "kill -STOP \$\$"
st=3
got TSTP
after
after
149
[1]+  Stopped                    bash -c "kill -TTIN \$\$"
bash -c "kill -TTIN \$\$"
bash -c "kill -STOP \$\$"
st=0
bash -c "kill -STOP \$\$; exit 4"
st=4
st=147
bash -c "kill -STOP \$\$; kill -STOP \$\$; exit 5"
st=147
[1]+  Stopped                    bash -c "kill -STOP \$\$; kill -STOP \$\$; exit 5"
bash -c "kill -STOP \$\$; kill -STOP \$\$; exit 5"
st=5
[1]+  Stopped                    sleep 1 > /dev/null 2>&1
-
-
-
[1]+  Running                    sleep 1 > /dev/null 2>&1 &
[1]?  Terminated                 sleep 1 > /dev/null 2>&1
bash: line 1: warning: wait_for_job: job 1 is stopped
st=0
[2]   Done                       ( sleep 0.3; kill -CONT $p )
st=0
[2]   Done                       ( sleep 0.2; kill -STOP $p; sleep 0.3; kill -CONT $p )
st=0
[2]   Done                       ( sleep 0.2; kill -STOP $p; sleep 0.3; kill -CONT $p ) > /dev/null 2>&1

== status 0
