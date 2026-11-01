# "$*" joins with the first character of IFS; unquoted $* and $@ split
# each parameter.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
set -- 'a b' c 'd:e'
show "$*"
IFS=:
show "$*"
show $*
show $@
IFS=':-'
show "$*"
IFS=
show "$*"
show $*
show $@
show "$@"
unset IFS
show "$*"
show $*
IFS=' '
set -- ' x ' '' y
show $*
show "$*"
show $@
