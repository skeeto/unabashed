# $0 $# $? $- and positional parameters
p() { printf '<%s>' "$@"; echo; }
p "$0" "${0}" "${0##*/}" "$#"
false; p "$?" "${?}"
(exit 7); p "$?"
p "$?"
set -- a b c d e f g h i j k
p "$1" "$9" "$10" "${10}" "${11}" "${12}" "$#"
shift 3; p "$1" "$#" "${8}" "${9}"
set --; p "$#" "$1"
p "$-"
set -f; p "$-"; set +f
set -u; p "$-"; set +u
set -e -f -u; p "$-"; set +efu
set -C; p "$-"; set +C
p "${-}" "${#-}"
