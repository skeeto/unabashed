v="x-y z"
unset IFS; printf '<%s>' $v${IFS:=-}$v; echo
unset IFS; printf '<%s>' ${IFS=-}$v; echo
unset IFS; printf '<%s>' $v${IFS=}$v; echo
set -- a 'b c' d
unset IFS; printf '<%s>' $*${IFS=-}$*; echo
unset IFS; printf '<%s>' $@${IFS=-}$@; echo
unset IFS; printf '<%s>' $*${IFS=}$*; echo
unset IFS; printf '<%s>' $v${IFS=}x$*; echo
unset IFS; printf '<%s>' ${x:-a b}${IFS=}x$*; echo
unset IFS; printf '<%s>' ${*}${IFS=}${*}${IFS:=-}"${*}"; echo
IFS=; printf '<%s>' $v$*; echo
IFS=; printf '<%s>' $v${IFS:=-}$v; echo
