== stdout 2075
bash: line 1: fg: no job control
st 1
bash: line 1: bg: no job control
st 1
bash: line 1: fg: no job control
st 1
bash: line 1: bg: no job control
st 1
bash: line 1: fg: no job control
st 1
bash: line 1: fg: job 1 started without job control
st 1
bash: line 1: bg: job 1 started without job control
st 1
bash: line 1: fg: current: no such job
st 1
bash: line 1: bg: current: no such job
st 1
bash: line 1: fg: %1: no such job
st 1
bash: line 1: fg: %1: no such job
st 1
bash: line 1: bg: %2: no such job
bash: line 1: bg: %3: no such job
st 1
bash: line 1: fg: -s: invalid option
fg: usage: fg [job_spec]
st 2
bash: line 1: bg: -x: invalid option
bg: usage: bg [job_spec ...]
st 2
bash: line 1: fg: current: no such job
st 1
bash: line 1: fg: %1: no such job
st 1
( sleep 1; exit 3 )
st 3
bash: line 1: bg: job 2 already in background
st 0
bash: line 1: bg: job 1 already in background
bash: line 1: bg: job 2 already in background
bash: line 1: bg: %3: no such job
st 1
sleep 1 | sleep 1
st 0
bash: line 1: bg: job 1 already in background
st 0
[1]+  Running                    sleep 1 &
sleep 1
st 0
bash: line 1: fg: job has terminated
st 1
[1]+  Done                       true
bash: line 1: fg: current: no such job
st 1
bash: line 1: fg: warning: 1: job specification requires leading `%'
sleep 1
st 0
sleep 1
st 0
sleep 1
st 0
sleep 1
st 0
bash: line 1: %1: command not found
st 0
bash: line 1: %1: command not found
st 127
bash: line 1: fg: no job control
st 1
bash: line 1: fg: no job control
bash: line 1: fg: no current jobs
[1]+  Running                    sleep 1 &
sleep 1
st 0
bash: line 1: fg: no current jobs
bash: line 1: bg: no current jobs
bash: line 1: fg: %nope: no such job
bash: line 1: fg: no current jobs

bash: line 1: fg: job 1 started without job control
bash: line 1: fg: current: no such job
st 1
sleep 1
st 0
f
sleep 1	(wd: ~)
sleep 3
Terminated                 sleep 3
st 143
sleep 2
usr1
st 0
+ fg
sleep 1
+ %1
bash: line 1: fg: %1: no such job
+ bg
bash: line 1: bg: current: no such job
bash: line 1: fg: no job control
st 1
sleep 1
st 127

== stderr 0

== merged 2075
bash: line 1: fg: no job control
st 1
bash: line 1: bg: no job control
st 1
bash: line 1: fg: no job control
st 1
bash: line 1: bg: no job control
st 1
bash: line 1: fg: no job control
st 1
bash: line 1: fg: job 1 started without job control
st 1
bash: line 1: bg: job 1 started without job control
st 1
bash: line 1: fg: current: no such job
st 1
bash: line 1: bg: current: no such job
st 1
bash: line 1: fg: %1: no such job
st 1
bash: line 1: fg: %1: no such job
st 1
bash: line 1: bg: %2: no such job
bash: line 1: bg: %3: no such job
st 1
bash: line 1: fg: -s: invalid option
fg: usage: fg [job_spec]
st 2
bash: line 1: bg: -x: invalid option
bg: usage: bg [job_spec ...]
st 2
bash: line 1: fg: current: no such job
st 1
bash: line 1: fg: %1: no such job
st 1
( sleep 1; exit 3 )
st 3
bash: line 1: bg: job 2 already in background
st 0
bash: line 1: bg: job 1 already in background
bash: line 1: bg: job 2 already in background
bash: line 1: bg: %3: no such job
st 1
sleep 1 | sleep 1
st 0
bash: line 1: bg: job 1 already in background
st 0
[1]+  Running                    sleep 1 &
sleep 1
st 0
bash: line 1: fg: job has terminated
st 1
[1]+  Done                       true
bash: line 1: fg: current: no such job
st 1
bash: line 1: fg: warning: 1: job specification requires leading `%'
sleep 1
st 0
sleep 1
st 0
sleep 1
st 0
sleep 1
st 0
bash: line 1: %1: command not found
st 0
bash: line 1: %1: command not found
st 127
bash: line 1: fg: no job control
st 1
bash: line 1: fg: no job control
bash: line 1: fg: no current jobs
[1]+  Running                    sleep 1 &
sleep 1
st 0
bash: line 1: fg: no current jobs
bash: line 1: bg: no current jobs
bash: line 1: fg: %nope: no such job
bash: line 1: fg: no current jobs

bash: line 1: fg: job 1 started without job control
bash: line 1: fg: current: no such job
st 1
sleep 1
st 0
f
sleep 1	(wd: ~)
sleep 3
Terminated                 sleep 3
st 143
sleep 2
usr1
st 0
+ fg
sleep 1
+ %1
bash: line 1: fg: %1: no such job
+ bg
bash: line 1: bg: current: no such job
bash: line 1: fg: no job control
st 1
sleep 1
st 127

== status 0
