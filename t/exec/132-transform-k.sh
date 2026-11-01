# ${x@K} ${x@k} on scalars and ${x@P}
p() { printf '<%s>' "$@"; echo; }
s='a b'; t="it's"; unset u
p "${s@K}" "${t@K}" "${u@K}"
p "${s@k}" ${s@k}
v='lit'; p "${v@P}"
w='back\slash'; p "${w@P}"
