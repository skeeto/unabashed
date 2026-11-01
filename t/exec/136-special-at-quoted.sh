# "$@" in assorted quoted contexts
p() { printf '<%s>' "$@"; echo; }
set -- a 'b c' ''
p "$@"
p "x$@y"
p "$@$@"
p "${@}"
p "-$@-" end
p "$@" ""
set --
p "$@"
p "$@" x
p x"$@"
p "$@"""
p """$@"
p "${@}" "${@:-}" 
p "x$@"
p "$@$@"
