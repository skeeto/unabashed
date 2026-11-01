t() { "$@"; echo "$? <- $*"; }
t test -o noglob
set -f
t test -o noglob
set +f
t test -o errexit
t test -o nosuchopt
t test ! -o noglob
x=1 e=
t test -v x
t test -v e
t test -v nosuch
unset x
t test -v x
set -- a b
t test -v 1
t test -v 2
t test -v 3
t test -v 0
t test -v '#'
t test -v '?'
t [[ -v e ]]
t [[ -v 2 ]]
t [[ -o noglob ]]
t [[ ! -v nosuch ]]
