# A pipe from shell code given to a program stays one pipe: what the
# program leaves unread is there for the next reader, whether the shell,
# another program, or a subshell's copy of the descriptor
printf '%s\n' a b c | { sleep 0; read x; echo "x=$x"; cat; }
printf '%s\n' 1 2 3 | { sleep 0 </dev/stdin; read x; echo "after sleep: $x"; }
printf '%s\n' p q | { { sleep 0; } | cat; read x; echo "after stage: $x"; }
printf '%s\n' u v | { (sleep 0); read x; echo "after subshell: $x"; }
printf '%s\n' m n o | { read x; wc -l; }
printf '%s\n' i j | { ls /nonexistent 2>/dev/null; read x; read y; echo "$x$y"; }
{ echo first; echo second; } | { sleep 0 & read x; echo "bg left: $x"; }
f() { while read l; do echo "<$l>"; sleep 0; done; }; printf '%s\n' 1 2 3 | f
