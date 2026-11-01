# BASH_ALIASES, BASH_CMDS, DIRSTACK, SHELLOPTS, BASHOPTS
echo "[${BASH_CMDS[@]}] [${!BASH_ALIASES[@]}]"
hash -p /usr/sbin/foo foo
hash -p /usr/local/bin/qux qux
echo ${!BASH_CMDS[@]} : ${BASH_CMDS[@]}
BASH_CMDS[blat]=/sbin/blat
hash
unset 'BASH_CMDS[qux]'; hash
alias sh='/bin/bash --login -o posix' blat='cd /blat'
BASH_ALIASES[foo]=/usr/sbin/foo
alias
echo ${!BASH_ALIASES[@]}
unset 'BASH_ALIASES[sh]'; alias
echo "${DIRSTACK[@]} ${#DIRSTACK[@]}"
echo "$SHELLOPTS"
echo "$BASHOPTS"
set -o errexit -o noglob; shopt -s extglob nullglob
echo "$SHELLOPTS"; echo "$BASHOPTS"
set +o errexit +o noglob; shopt -u extglob nullglob
SHELLOPTS=x; echo notreached
