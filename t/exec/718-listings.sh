# set, declare -p, readonly and export listings, lazy dynamic variables
declare -p BASHPID RANDOM LINENO BASH_COMMAND HISTCMD OPTIND SHELLOPTS 2>&1 | sed 's/=.*//'
declare -p | grep -E '^declare -[-a-zA-Z]+ (LINENO|BASH_SUBSHELL|HISTCMD|GROUPS|DIRSTACK|BASH_ALIASES|BASH_CMDS|COMP_WORDBREAKS|OPTIND|SHELLOPTS)\b'
: $LINENO $GROUPS
declare -p | grep -E '^declare -[-a-zA-Z]+ (LINENO|GROUPS)\b'
set | grep -E '^(LINENO|GROUPS|DIRSTACK|BASH_CMDS)='
readonly r1=1 r2; readonly -a ra=(x y); export e1=1 e2; declare -rx rx=2
declare -A aa=([k]=v); readonly aa
readonly -p | grep -v -E 'BASH|EUID|PPID|UID'
export -p | grep -E ' (e1|e2|rx)'
set -o posix
readonly -p | grep -v -E 'BASH|EUID|PPID|UID'
readonly -a
export -p | grep -E ' (e1|e2|rx)'
f() { local -r lr=1; local; readonly -p | grep lr; }; f
set +o posix
readonly 'q[1]'; echo "st=$?"
declare +a ra; echo "st=$?"
declare -f nosuch; declare -fp nosuch; echo "st=$?"
