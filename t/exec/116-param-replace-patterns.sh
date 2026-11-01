# patterns from variables and quoted patterns in replacement
p() { printf '<%s>' "$@"; echo; }
x='a.b.c*d'
dot=.; star='*'
p "${x//$dot/_}" "${x/$star/X}" "${x/"$star"/X}" "${x/\*/X}" "${x/'*'/X}"
p "${x//[.*]/_}" "${x//"."/_}"
q='?'; z='a?b'
p "${z/$q/Q}" "${z/"$q"/Q}" "${z//?/Q}"
long='aaaa'
p "${long/a*/X}" "${long//a/XY}" "${long/#a/}" "${long/%a/}"
p "${long//aa/b}"
