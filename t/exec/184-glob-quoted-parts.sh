# Quoted glob characters are literal; unquoted ones in the same word
# still match.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > 'a*b'; : > axb; : > 'a?'; : > ab; : > '[x]'; : > x
show a'*'b
show "a*"b
show a\*b
show a*b
show 'a'*
show "a"?
show a\?
show "a?"
show '[x]'
show [x]
show \[x]
show "["x"]"
show [\x]
show *'?'*
show "$(echo a)"*
