# time and ! on pipelines: the report, $?, PIPESTATUS and set -e
TIMEFORMAT='[t]'
time yes | head -1; echo "1 $? ${PIPESTATUS[*]}"
time false | true; echo "2 $? ${PIPESTATUS[*]}"
time ! false | false; echo "3 $? ${PIPESTATUS[*]}"
! time true | false; echo "4 $? ${PIPESTATUS[*]}"
set -o pipefail
time false | true; echo "5 $? ${PIPESTATUS[*]}"
time true | (exit 3) | true; echo "6 $?"
! false | true; echo "7 $? ${PIPESTATUS[*]}"
set +o pipefail
time { echo in | cat; } 2>/dev/null; echo "8"
{ time echo x | cat; } 2>&1 | cat
time -p true 2>&1
f() { time echo fn | tr a-z A-Z; }; f
true | time false; echo "9 $?"
time while false; do :; done; echo "10 $?"
x=$( time echo sub | cat ); echo "11 $x"
time (exit 4) | true; echo "12 ${PIPESTATUS[*]}"
set -e
time false | true; echo "13 survived"
! true; echo "14 survived $?"
! false | false; echo "15 survived $?"
