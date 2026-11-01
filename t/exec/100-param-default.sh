# ${x-w} ${x:-w} on unset, empty and set values
p() { printf '<%s>' "$@"; echo; }
unset u; e=; s=val
p "${u-a}" "${u:-a}" "${e-a}" "${e:-a}" "${s-a}" "${s:-a}"
p ${u-} ${u:-} "${u-}" "${u:-}"
p ${e-x} ${e:-x}
p "${u-$s}" "${u:-${e:-$s}}"
echo "u=${u-unset} e=${e-unset}"
