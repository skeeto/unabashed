# set -u with positional and special parameters
set -u
echo "[$#] [$*] [$@] [${@}] [${*}]"
p() { printf '<%s>' "$@"; echo; }
p "$@" end
p ${@+"$@"} end
( echo "$1"; echo after ); echo "status $?"
( echo "${1}"; echo after ); echo "status $?"
( echo "${10}"; echo after ); echo "status $?"
( echo "${#1}"; echo after ); echo "status $?"
echo "[${1-def}] [${1:-d2}]"
set -- a
echo "[$1] [${#@}] [${#*}]"
( echo "$2"; echo after ); echo "status $?"
( echo "[$!]"; echo after ); echo "status $?"
echo end
