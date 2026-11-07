== stdout 971
bash: cannot set terminal process group (N): Inappropriate ioctl for device
bash: no job control in this shell
himBHs
[1] PID
[1]+  Done                       sleep 0.1
[1] PID
[1]+  Done                       sleep 0.1
[1] PID
[1]+  Done                       true
[1] PID
[1]+  Exit 3                     exit 3
st=3
[1] PID
[1]+  Terminated                 sleep 5
[1] PID
[1]+  Killed                     sleep 5
next
[1] PID
[2] PID
[2]+  Done                       sleep 0.2
[1]   Done                       sleep 0.1
w=0
[1] PID
[2] PID
[1]-  Done                       sleep 0.1
n=0
[2]+  Running                    sleep 0.2 &
[2]+  Done                       sleep 0.2
[1] PID
[1]+  Exit 1                     false
bash: wait: pid N is not a child of this shell
last=127
[1] PID
posix
exit
---
bash: cannot set terminal process group (N): Inappropriate ioctl for device
bash: no job control in this shell
hiBHs
[1] PID
[1] PID
[1] PID
[1] PID
[1] PID
end
exit

== stderr 0

== merged 971
bash: cannot set terminal process group (N): Inappropriate ioctl for device
bash: no job control in this shell
himBHs
[1] PID
[1]+  Done                       sleep 0.1
[1] PID
[1]+  Done                       sleep 0.1
[1] PID
[1]+  Done                       true
[1] PID
[1]+  Exit 3                     exit 3
st=3
[1] PID
[1]+  Terminated                 sleep 5
[1] PID
[1]+  Killed                     sleep 5
next
[1] PID
[2] PID
[2]+  Done                       sleep 0.2
[1]   Done                       sleep 0.1
w=0
[1] PID
[2] PID
[1]-  Done                       sleep 0.1
n=0
[2]+  Running                    sleep 0.2 &
[2]+  Done                       sleep 0.2
[1] PID
[1]+  Exit 1                     false
bash: wait: pid N is not a child of this shell
last=127
[1] PID
posix
exit
---
bash: cannot set terminal process group (N): Inappropriate ioctl for device
bash: no job control in this shell
hiBHs
[1] PID
[1] PID
[1] PID
[1] PID
[1] PID
end
exit

== status 0
