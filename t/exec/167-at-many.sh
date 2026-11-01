# "$@" expands each parameter to a field; text before attaches to the
# first and text after to the last.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
set -- 'a b'
show "$@"
show "<$@>"
show pre"$@"post
set -- 'a b' c '' 'd  e'
show "$@"
show "<$@>"
show x"$@"y
show "$@""$@"
show "$@" $@
show $@
show "${@}"
show "${@}x${@}"
show "$1$@"
show "$@$#"
set -- ''
show "$@"
show x"$@"
show $@
set -- '' ''
show "$@"
show $@
