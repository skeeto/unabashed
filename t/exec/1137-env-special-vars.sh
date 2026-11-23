# Special variables from the environment, and in it, as bash has them

# An array bash would make dynamic is the plain variable inherited
env FUNCNAME=q BASH_SOURCE=s BASH_LINENO=5 BASH_ARGV=r BASH_ARGC=3 bash -c '
f() { echo "F=${FUNCNAME[*]} S=${BASH_SOURCE[*]} L=${BASH_LINENO[*]} AV=${BASH_ARGV[*]} AC=${BASH_ARGC[*]}"; caller 0; echo "caller $?"; }
f a b; declare -p FUNCNAME; printenv FUNCNAME'
env GROUPS=8 DIRSTACK=d BASH_CMDS=c BASH_ALIASES=a bash -c '
alias q=x; pushd / >/dev/null; declare -p GROUPS DIRSTACK BASH_CMDS BASH_ALIASES'
PIPESTATUS=7 bash -c 'false | true; echo "${PIPESTATUS[*]}"; PIPESTATUS[1]=8
false | true; echo "${PIPESTATUS[*]}"; declare -p PIPESTATUS'
# though a temporary binding of one that ignores assignments is none
FUNCNAME=q GROUPS=1 BASH_ARGC=3 printenv FUNCNAME GROUPS BASH_ARGC; echo "st $?"
h() { echo "in h ${FUNCNAME[0]}"; printenv FUNCNAME; echo "st $?"; }
FUNCNAME=q h
DIRSTACK=d printenv DIRSTACK

f() { local PIPESTATUS=5; false | true; echo "local ${PIPESTATUS[*]}"; }; f
g() { local PIPESTATUS; false | true; echo "local [${PIPESTATUS[*]}]"; }; g
false | true; echo "global ${PIPESTATUS[*]}"

# SECONDS counts on from its inherited value; the others drop theirs,
# and reach programs only once read
SECONDS=100 bash -c '(( SECONDS>=100 && SECONDS<110 )) && echo seconds'
SECONDS=abc bash -c 'echo "seconds $SECONDS"'
LINENO=7 BASHPID=3 BASH_SUBSHELL=9 HISTCMD=4 BASH_COMMAND=zz COMP_WORDBREAKS=zz bash -c '
echo "L=$LINENO BS=$BASH_SUBSHELL H=$HISTCMD BC=$BASH_COMMAND"
[ "$BASHPID" != 3 ] && echo pid; [ "$COMP_WORDBREAKS" != zz ] && echo wordbreaks'
RANDOM=1 SRANDOM=1 HISTCMD=1 bash -c 'printenv RANDOM SRANDOM HISTCMD; echo "st $?"
: $HISTCMD; printenv HISTCMD'

# Assigned, most stay dynamic
SRANDOM=9; [ "$SRANDOM" != 9 ] && echo srandom
EPOCHSECONDS=9; [ "$EPOCHSECONDS" != 9 ] && echo epochseconds
EPOCHREALTIME=9; [ "$EPOCHREALTIME" != 9 ] && echo epochrealtime
BASHPID=9; [ "$BASHPID" != 9 ] && echo bashpid
HISTCMD=9; echo "histcmd $HISTCMD"
BASH_SUBSHELL=4; echo "subshell $BASH_SUBSHELL $(echo $BASH_SUBSHELL)"; BASH_SUBSHELL=0
(LINENO[1]=x; declare -p LINENO)
(BASH_COMMAND[2]=x; declare -p BASH_COMMAND)
(COMP_WORDBREAKS=ab; echo "wordbreaks $COMP_WORDBREAKS")

# Programs get the environment as last made: again only once an
# exported variable changed, and then each entry as last made unless
# assigned since, LINENO computed anew
bash -c 'export LINENO
printenv LINENO
printenv LINENO
X=1 true
printenv LINENO
export Q=1
: | :
printenv LINENO
export R=1
{ :; } | { :; }
printenv LINENO
x=$(:)
printenv LINENO
f() { :; }
export S=1
X=1 f
printenv LINENO'
for v in BASH_SUBSHELL BASH_ARGV0 COMP_WORDBREAKS BASH_COMMAND HISTCMD SRANDOM; do
    V=$v bash -c 'e() { printenv $V || echo none; }
    export $V; eval "$V=9"; e; eval ": \$$V"; e; export X=1; e' | tr '\n' ' '
    echo
done
