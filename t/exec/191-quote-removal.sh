# Quote removal across mixed quoting styles.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show 'a'"b"c\d$'e'
show "'"'"'
show '"'"'"
show "a'b'c"
show 'a"b"c'
show "\'"
show '\'
show "$(echo '"q"')"
show $(echo '"q"')
show $(echo "'q'")
show "a""b" 'a''b'
show ""''""
show "\"\\"
x='"'
show $x "$x"
y="'a b'"
show $y
z='\a'
show $z "$z"
