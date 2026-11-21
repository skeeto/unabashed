# BASH_TRAPSIG is not bound over a readonly variable, and a trap run in
# a subshell leaves the readonly one's attributes as they were
trap 'declare -p BASH_TRAPSIG; printenv BASH_TRAPSIG || echo unexported' USR1
kill -USR1 $$
export BASH_TRAPSIG=5
readonly BASH_TRAPSIG
( trap 'echo "in trap $BASH_TRAPSIG"' USR1; kill -USR1 $BASHPID; : ) 2>/dev/null
declare -p BASH_TRAPSIG
( trap 'echo "exit $BASH_TRAPSIG"' EXIT; : ) 2>/dev/null
declare -p BASH_TRAPSIG
x=$(exec 2>/dev/null; trap 'echo "comsub $BASH_TRAPSIG"' EXIT; :)
echo "$x"
declare -p BASH_TRAPSIG
printenv BASH_TRAPSIG
