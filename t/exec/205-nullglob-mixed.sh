# nullglob with quoted and empty words alongside.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > m1
shopt -s nullglob
e=
show x* "$e" m*
show "" x*
show x*"" 
show x*''y
show \x*
p='x*'
show $p "$p"
show [
show ?
