# A lone backslash at the end of a word: unquoted it is removed, leaving
# a quoted empty field; inside double quotes it is kept.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
eval 'show a\'
eval 'show \'
eval 'show "a\"'
show {Z..a}
show x{Z..\\}y
eval 'show \' x
