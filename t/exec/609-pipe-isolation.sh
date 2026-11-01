# Every stage of a pipeline is a subshell: nothing it changes is seen
# afterward, except the last stage's under lastpipe
x=1; x=2 | true; echo "x=$x"
y=1; { y=2; echo "in y=$y"; } | cat; echo "out y=$y"
f() { echo f; }
{ f() { echo g; }; f; } | cat; f
true | { g() { echo h; }; }; type g 2>&1 | head -1
cd /tmp; cd / | true; pwd
set -- a b; set -- c | true; echo "$@"
z=0; echo hi | z=5; echo "z=$z"
arr=(1 2); arr[5]=x | cat; echo "${#arr[@]}"
export E=1; { export E=2; unset E; } | true; echo "E=$E"
alias ll='echo alias'; unalias ll | true; alias ll
echo "$BASH_SUBSHELL" | cat; { echo "$BASH_SUBSHELL"; } | cat
(echo "sub $BASH_SUBSHELL") | cat
{ echo a; exit 3; echo no; } | cat; echo "after=$?"
{ echo b; return 4 2>/dev/null; echo c; } | cat
h() { local l=inner; echo "$l" | { read v; echo "read $v $l"; }; echo "l=$l"; }
h
i=0; while [ $i -lt 3 ]; do i=$((i+1)); echo $i; done | tail -1; echo "i=$i"
shopt -s lastpipe
echo 7 | read q; echo "q=$q"
echo 8 | { read r; s=9; }; echo "r=$r s=$s"
