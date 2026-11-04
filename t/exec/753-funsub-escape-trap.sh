# Where an EXIT trap writes after an escape from ${ ...; }: exit and
# anything ending a subshell keep the substitution's redirections (the
# trap writes nowhere); an error at top level undoes them first
( trap 'echo sub-e trap' EXIT; set -e; x=${ exec 5>&1; false; echo no; }; echo no )
echo "st=$?"
( trap 'echo sub-div trap' EXIT; x=${ exec 5>&1; echo $((1/0)); }; echo "sub-div [$x] goes on" )
echo "st=$?"
( trap 'echo sub-unset trap' EXIT; x=${ : ${u?gone}; }; echo no )
echo "st=$?"
( f() { exit 7; }; trap 'echo sub-fn trap' EXIT; x=${ f; }; echo no )
echo "st=$?"
( trap 'echo sub-ret trap' EXIT; f() { x=${ return 3; }; echo "ret $?"; }; f )
echo "st=$?"
bash -c 'trap "echo c-exit trap" EXIT; x=${ exit 2; }'
echo "st=$?"
bash -c 'trap "echo c-unset trap" EXIT; x=${ : ${u?gone}; }'
echo "st=$?"
bash -c 'trap "echo c-e trap" EXIT; set -e; x=${ false; }; echo no'
echo "st=$?"
bash -c 'set -e; trap "echo c-e2 trap" EXIT; x=${ exec 2>&1; false; echo no; }; echo no' 2>&1
echo "st=$?"
trap 'echo top trap' EXIT
set -e
x=${ false; echo no; }
echo no
