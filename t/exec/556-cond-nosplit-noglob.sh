cd /tmp
: > a1
: > a2
t() { if eval "$1"; then echo "T: $1"; else echo "F: $1"; fi; }
x='a b'
t '[[ $x == "a b" ]]'
t '[[ $x == a\ b ]]'
t '[[ -n $x ]]'
t '[[ $x ]]'
star='*'
t '[[ $star == "*" ]]'
t '[[ a* == "a*" ]]'
t '[[ $star == a1 ]]'
g='a?'
t '[[ -e $g ]]'
t '[[ -f a* ]]'
t '[[ -f a1 ]]'
IFS=:
y='p:q'
t '[[ $y == "p:q" ]]'
t '[[ $y == p:q ]]'
unset IFS
w='  lead'
t '[[ $w == "  lead" ]]'
t '[[ $w == lead ]]'
nl='a
b'
t '[[ $nl == a?b ]]'
t '[[ $(echo "a  b") == "a  b" ]]'
t '[[ "$@" == "" ]]'
set -- one two
t '[[ $* == "one two" ]]'
t '[[ "$@" == "one two" ]]'
t '[[ $@ == one* ]]'
