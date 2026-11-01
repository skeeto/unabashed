# $@ inside operator words, quoted and unquoted
p() { printf '<%s>' "$@"; echo; }
set -- a 'b c'
unset u; s=set
p "${u:-$@}"
p "${u:-"$@"}"
p ${u:-"$@"}
p ${u:-$@}
p "${s:+$@}"
p ${s:+"$@"} end
p "${u-x$@y}"
p "${u:-$*}"
IFS=:; p "${u:-$*}" ${u:-$*}; unset IFS
set --
p "${u:-"$@"}" end
p ${u:-"$@"} end
