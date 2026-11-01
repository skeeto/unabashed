# ${x@Q}
p() { printf '<%s>' "$@"; echo; }
a=plain; b='it'\''s'; c='a b'; d=; unset u
p "${a@Q}" "${b@Q}" "${c@Q}" "${d@Q}" "${u@Q}"
e=$'tab\there'; f=$'nl\n'; g=$'\xc3\xa9'; h=$'\x01'
p "${e@Q}" "${f@Q}" "${g@Q}" "${h@Q}"
i='$HOME `x` "q" \'; p "${i@Q}"
p ${c@Q}
eval "v=${b@Q}"; p "$v"
set -- one 'two words' ''
p "${@@Q}"
p "${*@Q}"
p ${@@Q}
