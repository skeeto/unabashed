# alias is one of bash's assignment builtins: an argument written as an
# assignment is neither split nor globbed, tilde expansion follows the
# assignment rules, and name=(...) assigns the array (to the visible
# variable) before alias sees just the name. As for the declaration
# builtins, the command word as written decides: quoted, expanded, or
# after builtin it does not count, nor after command outside POSIX mode.
: >ab; : >ac
x='a  b a*'
HOME=/h
alias q1=$x q2=a* q3={a,b} q4=~:a:~/x q5=a~ q6+=$x 'q7[1]'=$x
alias q1 q2 q3 q4 q5 q6+ 'q7[1]'
alias -- q8=$x; alias q8
'q9'=$x 2>/dev/null; alias 'q9'=$x 2>&1; alias q9
al=alias; e=
$al r1=$x 2>&1; "alias" r2=$x 2>&1; \alias r3=$x 2>&1
command alias r4=$x 2>&1; builtin alias r5=$x 2>&1
$e alias r6=$x
alias r1 r2 r3 r4 r5 r6
alias() { printf '<%s>' "$@"; echo; }
alias f1=$x
unset -f alias
alias c1=(a $((1+1))); echo "status $?"; declare -p c1
c2=(o); alias c2+=(c) c3=$x; echo "status $?"; declare -p c2; alias c3
f() { local -a c4=(l); alias c4=([3]=a b); declare -p c4; }
f; declare -p c4 2>&1
declare -A h
{ alias h=([k]=v x); echo notreached; }
{ alias c5[1]=(a b); echo notreached; }
echo "status $?"
set -o posix
command alias p1=$x; alias p1
command command alias p2=$x; alias p2
builtin alias p3=$x 2>&1; alias p3
