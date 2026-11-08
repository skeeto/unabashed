# The shell reads its own pipes a block ahead after a while: whatever it
# read ahead still goes first to the next reader, a program included
lines() { i=1; while [ $i -le $1 ]; do echo $i; i=$((i+1)); done; }
take() { n=0; while [ $n -lt $1 ] && read -r l; do n=$((n+1)); done; echo "took $n, last $l"; }
lines 100 | { take 40; head -n 3; take 5; cat | wc -l; }
lines 100 | { take 40; mapfile -t a; echo "${#a[@]} ${a[0]}"; }
lines 5000 | { take 30; "$U" -c 'read -r x; echo "child $x"'; read -r y; echo "then $y"; }
exec 3< <(lines 200)
take 50 <&3; head -n 2 <&3; read -r z <&3; echo "fd3 $z"; exec 3<&-
# an inherited pipe is read exactly: the child leaves the rest
lines 100 | { "$U" -c 'n=0; while [ $n -lt 40 ] && read -r l; do n=$((n+1)); done; echo "child took $n"'; read -r l; echo "parent $l"; }
# a loop starting a program on the pipe each time
lines 30 | { while read -r l; do head -n 1; done | wc -l; }
