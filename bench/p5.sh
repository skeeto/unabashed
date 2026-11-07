# P5 (POSIX): function calls, recursion and case matching
fib() { if [ $1 -lt 2 ]; then r=$1; return; fi; fib $(($1-1)); eval "a$1=\$r"; fib $(($1-2)); eval "r=\$((a$1+r))"; }
fib 20; echo $r
kind() { case $1 in *[!0-9]*) k=word;; ?|??) k=small;; *0) k=round;; *) k=num;; esac; }
i=0; n=0; while [ $i -lt 100000 ]; do kind $i; [ $k = round ] && n=$((n+1)); i=$((i+1)); done; echo $n
