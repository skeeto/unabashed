# <(list): a pipe from a concurrent subshell, named /dev/fd/N, opened
# by builtins, redirections and programs alike
echo <(true)
cat <(echo hi)
cat <(echo one) - <(echo three) <<<two
read a b < <(echo 1 2); echo "read: $a+$b"
n=0; while read l; do n=$((n+1)); echo "line $l"; done < <(printf '%s\n' a b c); echo "n=$n"
f() { echo "f got ${1%%[0-9]*}"; cat "$1"; }; f <(echo infunc)
x=$(cat <(echo inner)); echo "x=$x"
for p in <(echo first) <(echo second); do cat "$p"; done
cat <(cat <(echo nested))
diff <(printf '%s\n' a b) <(printf '%s\n' a c); echo "diff $?"
source <(echo 'echo sourced $# "$@"') p q
exec 3< <(echo kept); cat <&3; exec 3<&-
echo pipe | cat - <(echo procsub)
[ -p <(true) ] && echo is-a-pipe
cat <(echo {a,b}) <(echo "$((6*7))")
cat <(echo err >&2) 2>/dev/null
v=old; cat <(v=new; echo "inside $v"); echo "outside $v"
cat <(echo $BASH_SUBSHELL)
y=<(true); echo "assigned ${y%%[0-9]*}"
echo "<(not a procsub)" '<(nor this)'
true <(exit 3); wait $!; echo "wait status $?"
cat <(exit 4); wait; echo "plain wait $?"
cat < <(echo redirected); echo "after redirect $?"
head -1 <(yes); echo "head $?"
