# unquoted $@ and $* with nondefault IFS
p() { printf '<%s>' "$@"; echo; }
set -- 'a:b' 'c d' '' 'e::f'
IFS=:
p $@
p $*
IFS=' :'
p $@
p $*
IFS='-:'
p $@
p $*
IFS=
p $@
p $*
unset IFS
p $@
p $*
