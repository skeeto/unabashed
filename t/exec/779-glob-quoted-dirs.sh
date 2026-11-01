# Quoted glob characters in any component match literally, also in a
# directory part that has other wildcards.
cd /tmp && mkdir qd && cd qd
mkdir 'a*' ab '[x]' x '?'
touch 'a*/1' ab/2 '[x]/3' x/4 '?/5'
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show '"a*"/*'   "a*"/*
show 'a*/*'     a*/*
show '"[x]"/*'  "[x]"/*
show '[x]/*'    [x]/*
show '\[x]/*'   \[x]/*
show '"?"/*'    "?"/*
show '?/*'      ?/*
v='a*'
show '"$v"/*'   "$v"/*
show '$v/*'     $v/*
w='\[x]'
show '$w/*'     $w/*
