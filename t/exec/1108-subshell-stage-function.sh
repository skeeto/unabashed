# BASH_SUBSHELL in a pipeline stage. bash forks a simple stage before
# its words expand, which still see the shell's level, and counts the
# child as a subshell once a builtin or function runs there: what they
# run is a level down, as all of a compound stage is. A job's stages,
# which bash forks from the shell, are each at the job's own level,
# compound or not; lastpipe's last stage runs in the shell.
f() { echo "f $BASH_SUBSHELL $1"; }
g() { f "$1"; echo "g $BASH_SUBSHELL $(echo $BASH_SUBSHELL)"; }
h() { echo "h $BASH_SUBSHELL" | cat; }
k() ( echo "k $BASH_SUBSHELL" )
f a | cat
cat </dev/null | f b
echo "c $BASH_SUBSHELL" | cat
true | echo "d $BASH_SUBSHELL"
{ echo "e $BASH_SUBSHELL"; } | cat
true | ( echo "f $BASH_SUBSHELL" )
for i in 1; do echo "for $BASH_SUBSHELL"; done | cat
true | while :; do echo "while $BASH_SUBSHELL"; break; done
g h | cat
true | g i
true | declare -p BASH_SUBSHELL
true | eval 'echo "eval $BASH_SUBSHELL"'
true | source /dev/stdin <<<'echo "source $BASH_SUBSHELL"'
true | printf '%s\n' "printf $BASH_SUBSHELL"
true | echo "$(echo "cs $BASH_SUBSHELL")"
x=1 f j | cat
f k 2>&1 | cat
! f l | cat
true | builtin eval 'f m'
h | cat
true | h
k | cat
true | k
( f n ) | cat
( f o | cat )
echo "$(f p | cat)"
r=$(true | f q); echo "$r"
f r | f s | cat
trap 'echo "ret $BASH_SUBSHELL"' RETURN
true | f t
trap - RETURN
coproc f u; cat <&${COPROC[0]}; wait
# Jobs
f v & wait
echo "amp $BASH_SUBSHELL" & wait
echo "w $BASH_SUBSHELL" | cat & wait
true | echo "x $BASH_SUBSHELL" & wait
{ echo "y $BASH_SUBSHELL"; } | cat & wait
( echo "z $BASH_SUBSHELL" ) | cat & wait
f A | cat & wait
true | f B & wait
true | declare -p BASH_SUBSHELL & wait
true | f C | cat & wait
( f D | cat ) & wait
{ true | f E; } & wait
r=$(f F | cat & wait); echo "$r"
shopt -s lastpipe
true | f G
true | { echo "lastpipe $BASH_SUBSHELL"; }
true | g H
f I | cat
