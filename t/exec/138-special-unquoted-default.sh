# unquoted $@ and $* with the default IFS
p() { printf '<%s>' "$@"; echo; }
set -- a 'b  c' '' ' d ' 'e'
p $@
p $*
p x$@y
p x$*y
p ${@} ${*}
set -- '' ''
p $@ end
p $* end
set --
p $@ $* end
