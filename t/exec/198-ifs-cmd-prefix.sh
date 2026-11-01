# IFS assigned as a command prefix does not affect splitting of that
# command's own arguments; it does affect builtins that use IFS.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
x='a:b c'
IFS=: show $x
IFS=: eval 'show $x'
show $x
f() { show $x; }
IFS=: f
IFS=: read -r p q <<< 'one:two three'
show "$p" "$q"
read -r p q <<< 'one:two three'
show "$p" "$q"
