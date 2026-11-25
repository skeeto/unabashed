# A {var} redirection's errors of bash's own (ambiguous, noclobber,
# restricted) name the variable, not the word; a dup of a bad
# descriptor first says it cannot duplicate it, naming the source. A
# move to {var} makes a new descriptor and closes the source, while
# only {var}>&- closes the one var names.
cd "$(mktemp -d)"
echo hi {fd}>$u
echo hi {fd}<$u
echo hi {fd}>>$u
echo hi {fd}<>$u
echo hi {fd}>&$u
echo hi {fd}<&$u
u='a b'; echo hi {fd}>$u; unset u
cat {fd}>$u
{fd}>$u
echo hi {fd}>&file
echo hi {fd}>/nope/x
echo hi >$u
fd=x; echo hi {fd}>&-; unset fd
v=7; echo hi {fd}>&$v
echo hi {fd}>&7-
echo hi {fd}<&7
echo hi {fd}<&$v-
echo hi 3>&7-
set -C; : >ex; echo hi {fd}>ex; echo hi >ex; set +C
bash -rc 'echo hi {fd}>f; echo hi {fd}>>f; echo hi {fd}<>f; echo hi >f'
exec 7>f; exec {fd}>&7-; echo "fd=$fd"; echo hi >&$fd; cat f; echo x >&7
exec 8<f; exec {g}<&8-; echo "g=$g"; read l <&$g; echo "$l"; read l <&8
exec 5>f; : {k}>&5-; echo "k=$k"; echo five >&5; cat f
x=3; exec {x}>&4-; echo "status $? x=$x"
exec {fd}>&-; echo hi >&$fd
