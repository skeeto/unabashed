# ${!name} indirection
p() { printf '<%s>' "$@"; echo; }
target=value; ref=target; unset u
p "${!ref}" "${!ref:-def}" "${!ref#va}" "${!ref/l/L}" "${#target}"
empty=; ref2=empty; p "${!ref2:-def}" "${!ref2-def}" "${!ref2:+alt}"
ref3=nosuch; p "${!ref3}" "${!ref3-unset}" "${!ref3:=assigned}" "$nosuch"
set -- HOME b c
p "${!1}" 
n=3; p "${!n}"
p "${!#}"
two=2; p "${!two}"
r=HOME; p "${!r:0:5}" "${!r^^}"
