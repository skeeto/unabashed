# $'...' translates C escapes.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
q() { printf '%q\n' "$1"; }
q $'a\tb'
printf '%s\n' $'a\nb'
show $'\x41\x42' $'\101\102' $'\u0043' $'\U00000044'
q $'\e\E'
show $'\'' $'\"' $'\\' $'\?'
q $'\a\b\f\v\r'
q $'\cA\cZ\c?\ca'
show $'\q\z'
show $'x\101y'
show $'\x4g'
show $'\x'
show $'\0101'
show $'\1010'
show $'a\0b' end
show $'a b'
show $'' x
show $''$''
show $'*'
show $'$HOME'
show "$'x'"
show '$'"'y'"
