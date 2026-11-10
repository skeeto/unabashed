== stdout 1115
declare -i BASHPID
declare -i RANDOM
declare -- LINENO
declare -- BASH_COMMAND
declare -i HISTCMD
declare -i OPTIND
declare -r SHELLOPTS
declare -A BASH_ALIASES=()
declare -A BASH_CMDS=()
declare -- BASH_SUBSHELL
declare -- COMP_WORDBREAKS
declare -a DIRSTACK=()
declare -a GROUPS=()
declare -i HISTCMD
declare -- LINENO
declare -i OPTIND="1"
declare -r SHELLOPTS="braceexpand:hashall:interactive-comments"
declare -a GROUPS=([0]="1000")
declare -- LINENO="4"
BASH_CMDS=()
DIRSTACK=()
GROUPS=([0]="1000")
LINENO=4
declare -r SHELLOPTS="braceexpand:hashall:interactive-comments"
declare -Ar aa=([k]="v" )
declare -r r1="1"
declare -r r2
declare -ar ra=([0]="x" [1]="y")
declare -rx rx="2"
declare -x e1="1"
declare -x e2
declare -rx rx="2"
readonly SHELLOPTS="braceexpand:hashall:interactive-comments:posix"
readonly -A aa=([k]="v" )
readonly r1="1"
readonly r2
readonly -a ra=([0]="x" [1]="y")
readonly rx="2"
readonly -a BASH_VERSINFO=([0]="5" [1]="3" [2]="0" [3]="1" [4]="release" [5]="HOST")
readonly -a ra=([0]="x" [1]="y")
export e1="1"
export e2
export rx="2"
declare -r lr="1"
readonly lr="1"
st=1
st=1
st=1

== stderr 144
bash: line 17: readonly: `q[1]': not a valid identifier
bash: line 18: declare: ra: readonly variable
bash: line 19: declare: nosuch: not found

== merged 1259
declare -i BASHPID
declare -i RANDOM
declare -- LINENO
declare -- BASH_COMMAND
declare -i HISTCMD
declare -i OPTIND
declare -r SHELLOPTS
declare -A BASH_ALIASES=()
declare -A BASH_CMDS=()
declare -- BASH_SUBSHELL
declare -- COMP_WORDBREAKS
declare -a DIRSTACK=()
declare -a GROUPS=()
declare -i HISTCMD
declare -- LINENO
declare -i OPTIND="1"
declare -r SHELLOPTS="braceexpand:hashall:interactive-comments"
declare -a GROUPS=([0]="1000")
declare -- LINENO="4"
BASH_CMDS=()
DIRSTACK=()
GROUPS=([0]="1000")
LINENO=4
declare -r SHELLOPTS="braceexpand:hashall:interactive-comments"
declare -Ar aa=([k]="v" )
declare -r r1="1"
declare -r r2
declare -ar ra=([0]="x" [1]="y")
declare -rx rx="2"
declare -x e1="1"
declare -x e2
declare -rx rx="2"
readonly SHELLOPTS="braceexpand:hashall:interactive-comments:posix"
readonly -A aa=([k]="v" )
readonly r1="1"
readonly r2
readonly -a ra=([0]="x" [1]="y")
readonly rx="2"
readonly -a BASH_VERSINFO=([0]="5" [1]="3" [2]="0" [3]="1" [4]="release" [5]="HOST")
readonly -a ra=([0]="x" [1]="y")
export e1="1"
export e2
export rx="2"
declare -r lr="1"
readonly lr="1"
bash: line 17: readonly: `q[1]': not a valid identifier
st=1
bash: line 18: declare: ra: readonly variable
st=1
bash: line 19: declare: nosuch: not found
st=1

== status 0
