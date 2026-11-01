# A writer to a pipe without readers dies as by SIGPIPE: status 141,
# silently, whether a program, a shell stage or a nested subshell
yes | head -1; echo "a ${PIPESTATUS[*]}"
{ yes; } | { read x; echo "got $x"; }; echo "b ${PIPESTATUS[*]}"
{ yes; echo "after $?" >&2; } | { read x; }; echo "c ${PIPESTATUS[*]}"
{ while :; do echo y; done; echo never >&2; } | head -1; echo "d ${PIPESTATUS[*]}"
{ while :; do printf 'y\n'; done; } | head -1; echo "e ${PIPESTATUS[*]}"
f() { while :; do echo f; done; }; f | { read x; echo "$x"; }; echo "f ${PIPESTATUS[*]}"
yes | yes | head -1; echo "g ${PIPESTATUS[*]}"
( yes ) | head -1; echo "h ${PIPESTATUS[*]}"
yes | cat | head -1; echo "i ${PIPESTATUS[*]}"
set -o pipefail
yes | head -1; echo "j $? ${PIPESTATUS[*]}"
set +o pipefail
x=$(yes | head -2); echo "k $x ${PIPESTATUS[*]}"
yes | (exec head -1); echo "l ${PIPESTATUS[*]}"
seq 1 | head -1; echo "m ${PIPESTATUS[*]}"
yes | true; echo "n ${PIPESTATUS[*]}"
shopt -s lastpipe
yes | read x; echo "p $x ${PIPESTATUS[*]}"
yes | head -1 | read y; echo "q $y ${PIPESTATUS[*]}"
yes | { yes | head -1; echo "inner ${PIPESTATUS[*]}"; } | head -1; echo "r ${PIPESTATUS[0]} ${PIPESTATUS[2]}"  # the middle races
{ printf '%s\n' a b; yes; } | head -3; echo "s ${PIPESTATUS[*]}"
yes | head -1 >/dev/null; echo "t $?"
f() { yes; echo "f after $?" >&2; return 3; }; f | head -1 >/dev/null; echo "u ${PIPESTATUS[*]}"
( set -o pipefail; yes | head -1 >/dev/null; echo "v $?" )
( set -eo pipefail; yes | head -1 >/dev/null; echo "never" ); echo "w $?"
( set -e; yes | head -1 >/dev/null; echo "x survived" )
