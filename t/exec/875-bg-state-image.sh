# A background job sees the whole state of the shell that started it,
# as a forked bash does: every listing is the same in the job as in its
# parent (inside functions called from a sourced file, with locals)
shopt -s extglob nullglob
set -o noclobber
alias ll='ls -l' la="ls -a"
declare -a arr=(a "b c" [7]=d)
declare -A as=([k1]=v1 [k2]="v 2" [x]=y [long key]=$'tab\there')
declare -i n=42
declare -r ro=const
declare -x ex=exported
declare -l low=MiXeD
declare -n ref=arr
unset gone
hash -p /opt/fakes/bin/cat kat
trap 'echo int' INT
trap '' QUIT
trap 'echo bye' EXIT
OPTIND=1
getopts ab: opt -a -b x
cd /tmp
cd /home/oracle
listing() {
    declare -p | grep -v -E '^declare -[-a-zA-Z]* (BASHPID|BASH_SUBSHELL|RANDOM|SRANDOM|SECONDS|EPOCHSECONDS|EPOCHREALTIME|LINENO|BASH_COMMAND|PIPESTATUS|BASH_LINENO|HISTCMD|_)='
    declare -f
    alias
    trap -p
    set -o
    shopt
    hash
    echo "$- $# $* ${FUNCNAME[*]} ${BASH_SOURCE[*]} $PWD $OLDPWD $OPTIND $opt"
}
f() {
    local loc=inner
    local -a la=(1 2 3)
    local -A lassoc=([p]=q)
    g "$@"
}
g() {
    local deeper=yes
    listing >p.txt
    listing >c.txt &
    wait $!
    if cmp -s p.txt c.txt; then echo same; else diff p.txt c.txt; fi
    echo "args $# $*"
}
printf '%s\n' 'f one "two three"' >s.sh
. ./s.sh
grep -c . p.txt >/dev/null && echo listed
