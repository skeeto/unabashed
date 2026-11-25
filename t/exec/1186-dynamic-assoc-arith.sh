# BASH_ALIASES and BASH_CMDS are associative arrays everywhere, built
# from the alias and command tables when read: arithmetic and [[ -v ]]
# take their subscripts as keys, not indices, a bare name is the key
# "0" (and DIRSTACK's is element 0), and an assignment to the bare name
# defines that key, as bash's assign functions do (+= replaces it).
# So does an array declared -A that has no elements yet. Under set -u
# arithmetic reads an array that is set but has no element 0 as 0.
exec 2>&1
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c"
    echo "status $?"
done <<'EOF'
set -u; echo $((BASH_ALIASES[x]+0)) $((BASH_CMDS[x]+0))
alias x=1; echo $((BASH_ALIASES[x]+0)) $((BASH_ALIASES[y]+0))
alias x=3; y=x; echo $((BASH_ALIASES[$y]*2)) $((BASH_ALIASES[a b]+0))
hash -p 5 t; echo $((BASH_CMDS[t]+0))
hash -p /bin/true t; echo $((BASH_CMDS[t]+0))
x=5; alias x=1; echo $((BASH_ALIASES[x])) $((BASH_ALIASES[1+0]+0))
alias x=1; let 'BASH_ALIASES[x]+=1'; alias x
alias x=1; echo $(( BASH_ALIASES[x]++ )); alias x
alias x=1; [[ -v BASH_ALIASES[x] ]]; echo $?; [[ -v BASH_ALIASES[y] ]]; echo $?
alias x=1; shopt -s array_expand_once; [[ -v BASH_ALIASES[x] ]]; echo $?
alias 0=7; echo "<$BASH_ALIASES>" $((BASH_ALIASES)) ${#BASH_ALIASES} ${BASH_ALIASES:0:1}
alias 0=7; x=BASH_ALIASES; echo ${!x} $((x))
hash -p 5 0; echo "<$BASH_CMDS>" $((BASH_CMDS))
[[ -v BASH_ALIASES ]]; echo $?; alias 0=7; [[ -v BASH_ALIASES ]]; echo $?
set -u; echo "<$BASH_ALIASES>"
alias 0=7; BASH_ALIASES=8; alias 0; read -r BASH_ALIASES <<<9; alias 0
alias 0=1; BASH_ALIASES+=x; alias 0; (( BASH_ALIASES=3 )); alias 0
BASH_CMDS=/bin/ls; hash; echo "<$BASH_CMDS>"
f() { local BASH_ALIASES=3; echo "<$BASH_ALIASES>"; alias; }; f
cd /; pushd /tmp >/dev/null; echo "<$DIRSTACK>"; echo $((DIRSTACK))
declare -A m; echo $((m[a b]+0)) $((m[x]=3)) ${m[x]}
set -u; declare -A m=([a]=1); a=([1]=1); echo $((m)) $((a)) $((BASH_ALIASES))
set -u; f() { echo $((BASH_ARGV)) $((BASH_SOURCE)); }; f
set -u; declare -a e; echo $((e))
EOF
