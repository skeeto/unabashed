# Default IFS: runs of space, tab and newline are one delimiter, and
# leading and trailing whitespace produce no fields.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
x='  a   b  '
show $x
x=$'\t\ta\t\n b\n\n'
show $x
x='   '
show $x
show "$x"
x=$'a\n\nb'
show $x
show pre$x"post"
unset IFS
x=' one  two '
show $x
show ${x}end
