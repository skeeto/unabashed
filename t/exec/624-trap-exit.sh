# The EXIT trap: on exit, at the end, after set -e and fatal errors; the
# status it leaves; exit inside it; subshells, substitutions and stages
# run their own; redirections in force as bash leaves them
( trap - 0; trap 'exit 5' 0; exit 0 ); echo "1 $?"
( trap 'false; exit' EXIT; true ); echo "2 $?"
( trap 'false' EXIT; exit 3 ); echo "3 $?"
( trap 'echo "in trap $?"' EXIT; (exit 4) ); echo "4 $?"
( trap 'echo bye' EXIT; echo body ); echo "5 $?"
x=$(trap 'echo bye' EXIT; echo hi); echo "6 [$x]"
echo in | { trap 'echo stage exit' EXIT; cat; }
{ trap 'echo first stage' EXIT; echo out; } | cat
( trap 'echo outer' EXIT; ( trap 'echo inner' EXIT; exit 1 ); echo "inner $?" )
( trap 'echo set; trap "echo reset" EXIT' EXIT; exit 0 ); echo "7 $?"
( trap '' EXIT; exit 6 ); echo "8 $?"
( trap 'echo once' EXIT; trap 'echo twice' EXIT ); echo "9 $?"
( f() { exit 3; }; trap 'echo bye1' EXIT; f > /dev/null ); echo "10 $?"
( f() { false; }; set -e; trap 'echo bye2' EXIT; f > /dev/null ); echo "11 $?"
( trap 'echo bye3' EXIT; exit 2 > /dev/null ); echo "12 $?"
( trap 'echo bye4' EXIT; exec > /dev/null; exit 2 ); echo "13 $?"
( trap 'echo bye5' EXIT; echo ${x?} > /dev/null ); echo "14 $?"
( trap 'echo bye7' EXIT; false > /dev/null; set -e; false >/dev/null ); echo "15 $?"
( set -e; exit_handler() { echo "exit code: $(( $? != 0 ))"; }
  main() {( trap exit_handler 0; "$@" >> /dev/null 2>&1 )}
  notfound() { nosuchcommand; }
  main notfound; echo "not here" ); echo "16 $?"
( trap 'echo "trap $BASH_SUBSHELL"' EXIT; echo "body $BASH_SUBSHELL" ) | cat
trap 'echo "final $?"' EXIT
false
