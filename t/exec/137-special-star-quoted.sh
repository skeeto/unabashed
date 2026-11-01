# "$*" joined by the first character of IFS
p() { printf '<%s>' "$@"; echo; }
set -- a 'b c' '' d
p "$*"
IFS=:; p "$*"
IFS=' :'; p "$*"
IFS=':-'; p "$*"
IFS=; p "$*"
unset IFS; p "$*"
IFS=$'\t'; p "$*"
IFS=:; p "${*}" "x${*}y" "${*:2}" "${*:-def}"
set --; p "$*" "${*}"
