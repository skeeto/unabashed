# "$@" is not affected by IFS; unquoted $@ splits each parameter.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
set -- 'a:b' ' c ' 'd e'
IFS=:
show "$@"
show $@
show x$@y
IFS=
show $@
show x$@y
unset IFS
show $@
show x$@y
show $1$2
set -- '' 'x'
show $@
show $1
