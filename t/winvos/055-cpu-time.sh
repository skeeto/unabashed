# personality: native
# Hand-checked against bash 5.3's time_command and times: a command's
# CPU time is the shell's own and that of the children it reaped while
# the command ran (getrusage's RUSAGE_SELF and RUSAGE_CHILDREN), and
# its share of the CPU is in whole percents. burn.exe, the vos's,
# charges the CPU time it is given and takes as long; the shell's own
# CPU time is none in the vos.
TIMEFORMAT='%R %U %S %P'
time burn 300 100
time { burn 200 0; sleep 0.2; }
time burn 30 0 | burn 0 20
time (burn 100 0)
time x=$(burn 0 100)
burn 100 100 &
TIMEFORMAT='%U %S'  # reaped while it waits; started before
time wait
TIMEFORMAT='%lU %0S'
time burn 61000 999
times
set -o posix
unset TIMEFORMAT
time
