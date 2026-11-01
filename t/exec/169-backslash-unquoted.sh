# Backslash outside quotes quotes the next character, which never
# splits or globs; a backslash-newline is removed.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
show a\ b
show \a\b\c
show \\
show \\\\
show \'
show \"
show \$x
show \`
show a\
b
show \*
show \ 
show x\ \ y
show \~
show \{a,b\}
show \#notcomment
show #comment
show a\#b
