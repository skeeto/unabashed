== stdout 3641
trap -- 'echo g' EXIT
trap -- 'echo c' SIGHUP
trap -- 'echo a' SIGINT
trap -- 'echo d' SIGUSR1
trap -- 'echo b' SIGTERM
trap -- 'echo e' SIGRTMIN+1
trap -- 'echo f' SIGRTMAX-2
1
1
1
2
---
2
0
trap -- 'echo x' SIGUSR1
1
 1) SIGHUP	 2) SIGINT	 3) SIGQUIT	 4) SIGILL	 5) SIGTRAP
trap -- 'echo x' SIGUSR1
trap -- 'echo x' SIGUSR2
echo x
echo x
0
bash: line 19: trap: -P requires at least one signal name
2
0
0
2
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- '' 32
0
 1) SIGHUP	 2) SIGINT	 3) SIGQUIT	 4) SIGILL	 5) SIGTRAP
 6) SIGABRT	 7) SIGBUS	 8) SIGFPE	 9) SIGKILL	10) SIGUSR1
11) SIGSEGV	12) SIGUSR2	13) SIGPIPE	14) SIGALRM	15) SIGTERM
16) SIGSTKFLT	17) SIGCHLD	18) SIGCONT	19) SIGSTOP	20) SIGTSTP
21) SIGTTIN	22) SIGTTOU	23) SIGURG	24) SIGXCPU	25) SIGXFSZ
26) SIGVTALRM	27) SIGPROF	28) SIGWINCH	29) SIGIO	30) SIGPWR
31) SIGSYS	34) SIGRTMIN	35) SIGRTMIN+1	36) SIGRTMIN+2	37) SIGRTMIN+3
38) SIGRTMIN+4	39) SIGRTMIN+5	40) SIGRTMIN+6	41) SIGRTMIN+7	42) SIGRTMIN+8
43) SIGRTMIN+9	44) SIGRTMIN+10	45) SIGRTMIN+11	46) SIGRTMIN+12	47) SIGRTMIN+13
48) SIGRTMIN+14	49) SIGRTMIN+15	50) SIGRTMAX-14	51) SIGRTMAX-13	52) SIGRTMAX-12
53) SIGRTMAX-11	54) SIGRTMAX-10	55) SIGRTMAX-9	56) SIGRTMAX-8	57) SIGRTMAX-7
58) SIGRTMAX-6	59) SIGRTMAX-5	60) SIGRTMAX-4	61) SIGRTMAX-3	62) SIGRTMAX-2
63) SIGRTMAX-1	64) SIGRTMAX	
---1
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---2
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---3
trap -- 'echo b' SIGINT
trap -- '' SIGUSR2
trap -- '' 32
---4
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---5
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---6
trap -- '' 32
---7
survived
---8
trap -- 'echo it'\''s' SIGINT
trap -- '' SIGUSR2
trap -- '' 32
---9
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---10
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---11
EXIT-group.1

== stderr 410
bash: line 5: trap: FOO: invalid signal specification
bash: line 6: trap: 99: invalid signal specification
bash: line 7: trap: -5: invalid signal specification
bash: line 8: trap: -z: invalid option
trap: usage: trap [-Plp] [[action] signal_spec ...]
trap: usage: trap [-Plp] [[action] signal_spec ...]
bash: line 15: trap: FOO: invalid signal specification
trap: usage: trap [-Plp] [[action] signal_spec ...]

== merged 4051
trap -- 'echo g' EXIT
trap -- 'echo c' SIGHUP
trap -- 'echo a' SIGINT
trap -- 'echo d' SIGUSR1
trap -- 'echo b' SIGTERM
trap -- 'echo e' SIGRTMIN+1
trap -- 'echo f' SIGRTMAX-2
bash: line 5: trap: FOO: invalid signal specification
1
bash: line 6: trap: 99: invalid signal specification
1
bash: line 7: trap: -5: invalid signal specification
1
bash: line 8: trap: -z: invalid option
trap: usage: trap [-Plp] [[action] signal_spec ...]
2
---
trap: usage: trap [-Plp] [[action] signal_spec ...]
2
0
trap -- 'echo x' SIGUSR1
bash: line 15: trap: FOO: invalid signal specification
1
 1) SIGHUP	 2) SIGINT	 3) SIGQUIT	 4) SIGILL	 5) SIGTRAP
trap -- 'echo x' SIGUSR1
trap -- 'echo x' SIGUSR2
echo x
echo x
0
bash: line 19: trap: -P requires at least one signal name
2
0
0
trap: usage: trap [-Plp] [[action] signal_spec ...]
2
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- '' 32
0
 1) SIGHUP	 2) SIGINT	 3) SIGQUIT	 4) SIGILL	 5) SIGTRAP
 6) SIGABRT	 7) SIGBUS	 8) SIGFPE	 9) SIGKILL	10) SIGUSR1
11) SIGSEGV	12) SIGUSR2	13) SIGPIPE	14) SIGALRM	15) SIGTERM
16) SIGSTKFLT	17) SIGCHLD	18) SIGCONT	19) SIGSTOP	20) SIGTSTP
21) SIGTTIN	22) SIGTTOU	23) SIGURG	24) SIGXCPU	25) SIGXFSZ
26) SIGVTALRM	27) SIGPROF	28) SIGWINCH	29) SIGIO	30) SIGPWR
31) SIGSYS	34) SIGRTMIN	35) SIGRTMIN+1	36) SIGRTMIN+2	37) SIGRTMIN+3
38) SIGRTMIN+4	39) SIGRTMIN+5	40) SIGRTMIN+6	41) SIGRTMIN+7	42) SIGRTMIN+8
43) SIGRTMIN+9	44) SIGRTMIN+10	45) SIGRTMIN+11	46) SIGRTMIN+12	47) SIGRTMIN+13
48) SIGRTMIN+14	49) SIGRTMIN+15	50) SIGRTMAX-14	51) SIGRTMAX-13	52) SIGRTMAX-12
53) SIGRTMAX-11	54) SIGRTMAX-10	55) SIGRTMAX-9	56) SIGRTMAX-8	57) SIGRTMAX-7
58) SIGRTMAX-6	59) SIGRTMAX-5	60) SIGRTMAX-4	61) SIGRTMAX-3	62) SIGRTMAX-2
63) SIGRTMAX-1	64) SIGRTMAX	
---1
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---2
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---3
trap -- 'echo b' SIGINT
trap -- '' SIGUSR2
trap -- '' 32
---4
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---5
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---6
trap -- '' 32
---7
survived
---8
trap -- 'echo it'\''s' SIGINT
trap -- '' SIGUSR2
trap -- '' 32
---9
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---10
trap -- 'echo e' EXIT
trap -- 'echo it'\''s' SIGINT
trap -- 'echo k' SIGKILL
trap -- 'echo a' SIGUSR1
trap -- '' SIGUSR2
trap -- '' 32
trap -- 'echo w' SIGRTMIN
trap -- 'echo w' SIGRTMIN+15
trap -- 'echo w' SIGRTMAX-14
trap -- 'echo w' SIGRTMAX-10
trap -- 'echo w' SIGRTMAX
trap -- 'echo err' ERR
trap -- 'echo r' RETURN
---11
EXIT-group.1

== status 0
