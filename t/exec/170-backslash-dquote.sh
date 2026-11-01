# Inside double quotes a backslash quotes only $ ` " \ and newline;
# before anything else it is kept.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show "\a\b"
show "\\"
show "\$x"
show "\`"
show "\""
show "\'"
show "a\
b"
show "\n\t"
show "\*"
show "\{"
show "\~"
show '\\'
show "\\\\"
x='a\b'
show "$x" $x
show "${x}\x"
