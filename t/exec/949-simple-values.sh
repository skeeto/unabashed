# Assignment values, case words and [[ ]] operands that are a lone
# parameter, perhaps quoted, perhaps with - :- + :+ and a plain word:
# every kind of value, set -u, and a variable changed while in use
p() { printf '%s' "$#"; printf ' <%s>' "$@"; echo; }
e=; s=val; sp='a  b'; g='*'; bs='a\b'; unset u
declare -a arr=(x y); declare -A as=([0]=z); declare -i n=3
declare -n ref=s
for v in e s sp g bs u arr as n ref; do
    echo "== $v"
    eval 'a1=$'$v' a2="$'$v'" a3=${'$v'} a4=${'$v':-d} a5="${'$v'-d}"'
    eval 'a6=${'$v':+p} a7=${'$v'+p} a8="${'$v':-a b}"'
    p "$a1" "$a2" "$a3" "$a4" "$a5" "$a6" "$a7" "$a8"
    eval 'case $'$v' in "") echo empty;; \*) echo star;; *) echo other;; esac'
    eval 'case "${'$v':-a b}" in "a b") echo dflt;; *) echo set;; esac'
    eval '[[ $'$v' == "${'$v'}" ]] && echo same'
    eval '[[ "$'$v'" = ${'$v'-} ]] && echo same2 || echo differ2'
done
x=one
case $x in one) x=two; echo body;;& two) echo two;; one) echo still;; esac
case $x in two) unset x; echo unset;;& *) echo "next <$x>";; esac
n2=5; declare -i m; m=$n2+1; echo "m=$m"
y=$n2; y+=$s; echo "y=$y"
t=~; t2=$e~; t3=${u:-x}:~
[ "$t" = "$HOME" ] && echo tilde; echo "t2=$t2"
[ "${t3#x:}" = "$HOME" ] && echo tilde3
f() { local l=$1; case $l in a) echo A;; *) echo "<$l>";; esac; }
f a; f 'b  c'
for w in 1 2; do q=$w; echo "q=$q"; done
IFS=:; c=$sp; d=${u:-a:b}; echo "[$c] [$d]"; unset IFS
export ex=$s; echo "ex=$ex"
set -u
z=${u-ok}; echo "z=$z"; z=${u:+no}; echo "z=<$z>"
( z=$u; echo "not reached" ) 2>&1; echo "status $?"
( case $u in *) echo "not reached";; esac ) 2>&1; echo "status $?"
( [[ $u == x ]]; echo "not reached" ) 2>&1; echo "status $?"
set +u
readonly ro=$s; echo "ro=$ro"
( ro=$e ) 2>&1; echo "status $?"
