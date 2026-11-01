# A pattern with no match stays as written (after quote removal);
# nullglob removes it; set -f disables globbing.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > one
show *.none
show "*".none x*'y'
show z\*q
show [ab
show [
show ]
show a]
show *one
shopt -s nullglob
show *.none
show *.none x
show "*.none"
show *one
set -- *.none
echo $#
shopt -u nullglob
set -f
show * *one
x='*'
show $x
set +f
show $x
show "$x"
