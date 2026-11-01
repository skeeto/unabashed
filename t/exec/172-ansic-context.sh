# $'...' in assignments, ${...} words, and adjacent to other parts.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
q() { printf '%q\n' "$1"; }
a=$'1\t2'
q "$a"
show $a
b=x$'\n'y
q "$b"
q "${u-$'\t'}"
q ${u-$'a b'}
show ${u-$'a b'}
q "${u:-$'\x41'}"
c=pre$'mid'"dq"'sq'post
show "$c"
IFS=$'\n'
d=$'one two\nthree'
show $d
unset IFS
case $'\t' in $'\t') echo tab;; *) echo no;; esac
