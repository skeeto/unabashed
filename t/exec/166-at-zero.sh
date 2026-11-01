# "$@" with no positional parameters disappears entirely, unless the
# word has other quoted or unquoted parts.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
set --
show "$@"
show "$@" "$@"
show x"$@"
show "$@"y
show "x$@y"
show "$@"""
show """$@"
show ''"$@"
show "$@"$e
show "${@}"
show "${@:-d}"
show "$*"
show $@
show $*
show "${@}${@}"
show "$@""$@"
