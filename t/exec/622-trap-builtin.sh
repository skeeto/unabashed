# trap: setting, resetting and ignoring, signal specs, listing, -p -P -l,
# errors, and the parent's traps listed in subshells until they set one
trap 'echo a' int; trap 'echo b' sigterm; trap 'echo c' SIGHUP; trap 'echo d' 10; trap 'echo e' RTMIN+1; trap 'echo f' SIGRTMAX-2; trap 'echo g' exit
trap
trap 'x' FOO; echo $?
trap 'x' 99; echo $?
trap 'x' -5; echo $?
trap -z; echo $?
trap - int sigterm 1 10 SIGRTMIN+1 rtmax-2 0
trap
echo ---
trap 'echo hi' ; echo $?
trap USR1; echo $?
trap 'echo x' USR1 USR2
trap -p USR1 FOO; echo $?
trap -l 2>&1 | head -1
trap -p 2>&1
trap -P USR1 USR2 2>&1; echo $?
trap -P 2>&1; echo $?
trap 0 USR1 USR2; trap; echo $?
trap 'echo q' 0; trap 1 0; trap ; echo $?
trap -- -; echo $?; trap
trap 'echo w' SIGRTMIN+20 RTMIN+15 RTMAX-14 34 64; trap
trap "echo it's" INT; trap -p INT
trap 'echo k' KILL; trap -p KILL
trap '' 32; trap -p 32 33; echo $?
trap -l
trap 'echo a' USR1; trap '' USR2; trap 'echo e' EXIT; trap 'echo r' RETURN; trap 'echo err' ERR
echo ---1; ( trap )
echo ---2; ( ( trap ) )
echo ---3; ( trap 'echo b' INT; trap )
echo ---4; x=$(trap); echo "$x"
echo ---5; ( trap -p USR1 USR2; trap -p )
echo ---6; ( trap - USR2; trap )
echo ---7; ( kill -USR2 $BASHPID; echo survived )
echo ---8; ( trap -p INT; trap -- USR1; trap )
echo ---9; trap | cat
echo ---10; { trap; } | cat
echo ---11; trap 'echo USR1 $BASH_SUBSHELL' USR1; { trap 'echo EXIT-group.$BASH_SUBSHELL' EXIT; } | cat
trap - EXIT
