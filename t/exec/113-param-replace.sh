# ${x/p/r} and ${x//p/r}
p() { printf '<%s>' "$@"; echo; }
x=banana
p "${x/a/o}" "${x//a/o}" "${x/an/AN}" "${x//an/AN}"
p "${x/a}" "${x//a}" "${x/a/}" "${x//a/}"
p "${x/*/X}" "${x//?/.}" "${x/n*/N}" "${x/b*n/Q}"
p "${x/x/y}" "${x//[an]/_}" "${x//[!a]/-}"
p "${x/}" "${x//}" "${x///z}"
e=; p "${e/a/b}" "${e//*/b}"
unset u; p "${u/a/b}"
