# Assignment arguments of declaration builtins are not split or
# globbed, and get assignment tilde expansion.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > g1; : > g2
x='1  2'
declare a=$x
show "$a"
export b=$x c=g*
show "$b" "$c"
readonly r=$x:~
show "$r"
typeset t=~/t
show "$t"
f() { local l=$x m=~ n=g?; show "$l" "$m" "$n"; }
f
set -- p 'q r'
declare d=$@
show "$d"
declare e="$@"
show "$e"
declare $x 2>&1
declare -p g1 2>&1
z='y=1 2'
declare $z 2>&1
show "$y"
