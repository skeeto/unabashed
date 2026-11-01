# No splitting or globbing in assignments, case words, here-strings,
# and redirection targets.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > g1; : > g2
x='a   b'
y=$x
show "$y"
y=*
show "$y"
y=$x*
show "$y"
case $x in 'a   b') echo case-ok;; *) echo case-split;; esac
case g? in 'g?') echo case-lit;; *) echo case-glob;; esac
read -r r <<< $x
show "$r"
read -r r <<< g*
show "$r"
echo hi > $x
read -r r < "a   b"
show "$r"
f=g*
echo out > $f
read -r r < 'g*'
show "$r"
set -- 'p  q' r
y=$@
show "$y"
y=$*
show "$y"
IFS=:
y=$*
show "$y"
y="$*"
show "$y"
