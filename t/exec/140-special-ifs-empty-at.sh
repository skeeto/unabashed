# empty IFS with empty parameters and joined words
p() { printf '<%s>' "$@"; echo; }
IFS=
set -- a '' b
p $@
p $*
p x$@
p $@y
set -- '' ''
p $@
p $*
p "$@"
set -- 'a b' 'c'
p $*$@
