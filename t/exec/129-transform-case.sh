# ${x@U} ${x@L} ${x@u}
p() { printf '<%s>' "$@"; echo; }
x='hello World'; unset u
p "${x@U}" "${x@L}" "${x@u}" "${u@U}"
set -- one Two
p "${@@U}" "${*@L}" "${@@u}"
p "${1@U}" "${2@L}"
