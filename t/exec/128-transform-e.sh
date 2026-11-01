# ${x@E}
p() { printf '<%s>' "$@"; echo; }
a='a\tb'; b='\x41\101\0102'; c='q\"\'\''x'; d='\cA'; e='nl\n'
p "${a@E}" "${b@E}" "${c@E}" "${e@E}"
printf '%s' "${d@E}" | { read -r v; p "${#v}"; }
f='\e[0m'; f=${f@E}; p "${#f}"
g='end\'; p "${g@E}"
h='a\c b'; p "${h@E}"
unset u; p "${u@E}"
