# $@ and $* in non-splitting contexts
p() { printf '<%s>' "$@"; echo; }
set -- a 'b c' d
IFS=:
v=$@; p "$v"
v="$@"; p "$v"
v=$*; p "$v"
v="$*"; p "$v"
v=${*}; p "$v"
v=x$*y; p "$v"
case $* in a:b\ c:d) echo case-colon;; *) echo case-other;; esac
case $@ in "a b c d") echo case-space;; *) echo case-other;; esac
case "$*" in a:*) echo dq-colon;; esac
read -r w <<< $*; p "$w"
read -r w <<< "$@"; p "$w"
unset IFS
v=$*; p "$v"
IFS=; v=$*; p "$v"; v=$@; p "$v"
