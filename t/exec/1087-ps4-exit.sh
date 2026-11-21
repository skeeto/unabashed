# exit while PS4 expands under set -x (the builtin in ${ cmd; }, in a
# nested one, by set -e after an error) ends the shell there, with
# xtrace off for the EXIT trap, whose output to fd 1 goes into the
# ${ cmd; } as in bash; in a ( list ) only the subshell; an expansion
# error alone is reported and the raw text used
n=0
run() {
    n=$((n+1))
    printf '%s: ' $n
    bash -c "$1" 2>&1
    echo "st $?"
}
run 'trap "echo EXIT \$?" EXIT; PS4='"'"'${ exit 3; }+ '"'"'; set -x; echo a; echo b'
run 'trap "echo EXIT \$? \$- >&2" EXIT; PS4='"'"'${ exit 3; }+ '"'"'; set -x; echo a'
run 'PS4='"'"'${u?oops}+ '"'"'; set -x; echo a; set +x; echo c'
run 'f() { PS4='"'"'${ exit 4; }+ '"'"'; set -x; echo a; }; f; echo after $?'
run 'x=${ PS4='"'"'${ exit 5; }+ '"'"'; set -x; echo a; }; echo after $? "$x"'
run 'PS4='"'"'${ return 6; }+ '"'"'; f() { set -x; echo a; set +x; echo b; }; f; echo after $?'
run 'trap "echo EXIT \$?" EXIT; PS4='"'"'${ exit 3; }+ '"'"'; ( set -x; echo a ); echo after $?'
run 'PS4='"'"'${ exit; }+ '"'"'; false; set -x; echo a'
run 'PS4='"'"'${ exit 3; }+ '"'"'; set -x; x=1; echo b'
run 'PS4='"'"'${ exit 3; }+ '"'"'; for i in 1; do set -x; (( 1 )); echo b; done'
run 'PS4='"'"'${ exit 3; }+ '"'"'; set -x; [[ 1 ]]; echo b'
run 'PS4='"'"'${ eval exit 3; }+ '"'"'; set -x; echo a'
run 'shopt -s inherit_errexit; set -e; trap "echo EXIT \$? >&2" EXIT; PS4='"'"'${ false; }+ '"'"'; set -x; echo a'
run 'set -e; trap "echo EXIT \$? \$- >&2" EXIT; PS4='"'"'${u?oops}+ '"'"'; set -x; echo a'
run 'PS4='"'"'$(exit 7)+ '"'"'; set -x; echo a; echo $?'
