# operators applied to $@ and $*
p() { printf '<%s>' "$@"; echo; }
set -- apple 'big banana' cherry
p "${@#?}" "${@%a}"
p "${*#?}"
p "${@/a/A}"
p "${@//a/A}"
p ${@/a/A}
p "${@^}"
p "${*^^}"
p "${@:-x}" "${*:+set}"
set -- ''
p "${@:-x}" "${*:-y}" "${@-z}"
set -- '' ''
p "${@:-x}" "${*:-y}"
IFS=; p "${*:-y}"; unset IFS
set --
p "${@:-x}" "${@-z}" "${*:+set}" "${@+set}"
