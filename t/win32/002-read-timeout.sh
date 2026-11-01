# read -t on a pipe a program has not written to yet: the timeout ends
# the read, and nothing the program writes later is lost, whether the
# shell or a program reads it next.
slow() { "$U" -c "sleep $1; printf '$2'"; }
slow 2 'late\n' | { read -t 1 x; echo "st=$? x=[$x]"; read y; echo "y=$y"; }
slow 2 'l1\nl2\n' | { read -t 1 x; echo "st=$?"; cat; echo "cat $?"; }
slow 2 'l1\nl2\nl3\n' | { read -t 1 x; "$U" -c 'read a; echo "child $a"'; read b; echo "b=$b"; cat; }
# A readiness check while a timed-out read may still be in flight
slow 2 'z\n' | { read -t 1 x; read -t 0; echo "ready=$?"; read -t 0.3 y; echo "st=$?"; read z; echo "z=$z"; }
# Closing the descriptor after a timeout must not hang
exec 3< <("$U" -c 'sleep 2; echo q')
read -t 1 -u 3 x; echo "st=$?"
exec 3<&-
echo closed
# TMOUT is read's default timeout
slow 2 'w\n' | { TMOUT=1; read v; echo "st=$? v=[$v]"; unset TMOUT; read v; echo "v=$v"; }
