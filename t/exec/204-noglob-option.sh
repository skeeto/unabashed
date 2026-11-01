# set -f / set -o noglob disable globbing but not brace or tilde
# expansion or splitting.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > n1; : > n2
set -o noglob
show n* {n1,n2} ~
x='n* n?'
show $x
echo $-
set +o noglob
show n*
show $x
shopt -s nullglob
set -f
show n.none*
set +f
show n.none*
