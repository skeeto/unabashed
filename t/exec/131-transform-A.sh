# ${x@A} assignment form
p() { printf '<%s>' "$@"; echo; }
plain=val; sq="it's"; e=; unset u
p "${plain@A}" "${sq@A}" "${e@A}" "${u@A}"
declare -i int=5; p "${int@A}"
export ex=1; p "${ex@A}"
declare -rx rx=two; p "${rx@A}"
declare -l lo=Q; p "${lo@A}"
nl=$'a\nb'; p "${nl@A}"
set -- pos
p "${1@A}"
