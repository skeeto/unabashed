# ${x/#p/r} and ${x/%p/r}
p() { printf '<%s>' "$@"; echo; }
x=abcabc
p "${x/#abc/X}" "${x/%abc/X}" "${x/#b/X}" "${x/%b/X}"
p "${x/#/pre-}" "${x/%/-suf}" "${x/#*/all}" "${x/%*/all}"
p "${x/#a*b/X}" "${x/%b*/X}" "${x/#?/X}" "${x/%?/X}"
e=; p "${e/#/pre}" "${e/%/suf}"
p "${x/\#/X}" "y${x/\%/X}"
h='#a%'; p "${h/\#/H}" "${h/\%/P}" "${h/'#'/H}"
