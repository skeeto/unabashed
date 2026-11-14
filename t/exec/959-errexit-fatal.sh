# Under set -e an unbound variable or ${x?} ends the shell with status
# 1, even where set -e is ignored, as bash's report_error exits then
( : ${x?}; echo not reached ); echo "sub $?"
( set -e; f() { : ${x?}; }; f || echo caught ); echo "sub $?"
( set -eu; (( n + 1 )) || :; echo no ); echo "sub $?"
( set -eu; x=$nope ); echo "sub $?"
trap 'echo "exit $?"' EXIT
set -e
if true ${x:?no}; then echo then; fi
echo not reached
