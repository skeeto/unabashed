# IFS characters that are otherwise special: backslash, glob
# characters, quotes and digits.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
IFS='\'
x='a\b\\c'
show $x
IFS='*'
x='a*b**'
show $x
show "$x"
IFS='"'
x='a"b'
show $x
IFS=0
show $((100)) $((1+1))
x=$((10203))
show $x
IFS=a
show $x
v=banana
show $v
show ${v}
show "$v"
IFS=$'\n'
show $(printf 'l 1\nl 2\n')
