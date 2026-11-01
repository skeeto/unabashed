== stdout 1016
[1]   Running                    sleep 3 &
[2]-  Running                    f &
[3]+  Running                    { sleep 2; exit 5; } &
pid ok
jobs -p agrees
[1]   Running                    sleep 3 &
[3]+  Running                    { sleep 2; exit 5; } &
[2]-  Running                    f &
[3]+  Running                    { sleep 2; exit 5; } &
[1]   Running                    sleep 3 &
[3]+  Running                    { sleep 2; exit 5; } &
[2]-  Running                    f &
status 1
[1]   Running                    sleep 3 &
status 0
[1]   Running                    sleep 3 &
status 0
[1]   Running                    sleep 3 &
[2]-  Running                    f &
[3]+  Running                    { sleep 2; exit 5; } &
f ran
-- after
[1]   Done                       sleep 3
[2]-  Exit 3                     f
[3]+  Exit 5                     { sleep 2; exit 5; }
-- again
[1]   Terminated                 sleep 3
[2]+  User defined signal 2      sleep 3
-- end
[1]+  Done                       true

== stderr 110
bash: line 14: jobs: %3x: no such job
bash: line 16: jobs: warning: 1: job specification requires leading `%'

== merged 1126
[1]   Running                    sleep 3 &
[2]-  Running                    f &
[3]+  Running                    { sleep 2; exit 5; } &
pid ok
jobs -p agrees
[1]   Running                    sleep 3 &
[3]+  Running                    { sleep 2; exit 5; } &
[2]-  Running                    f &
[3]+  Running                    { sleep 2; exit 5; } &
[1]   Running                    sleep 3 &
[3]+  Running                    { sleep 2; exit 5; } &
[2]-  Running                    f &
bash: line 14: jobs: %3x: no such job
status 1
[1]   Running                    sleep 3 &
status 0
bash: line 16: jobs: warning: 1: job specification requires leading `%'
[1]   Running                    sleep 3 &
status 0
[1]   Running                    sleep 3 &
[2]-  Running                    f &
[3]+  Running                    { sleep 2; exit 5; } &
f ran
-- after
[1]   Done                       sleep 3
[2]-  Exit 3                     f
[3]+  Exit 5                     { sleep 2; exit 5; }
-- again
[1]   Terminated                 sleep 3
[2]+  User defined signal 2      sleep 3
-- end
[1]+  Done                       true

== status 0
