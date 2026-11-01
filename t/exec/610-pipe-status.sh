# Exit statuses of pipelines: the last stage's, or with pipefail the
# rightmost failure; every stage sees the $? from before the pipeline
true | false; echo $?
false | true; echo $?
false | false | true; echo $?
! true | false; echo $?
! false | true; echo $?
(exit 4); (exit 5) | echo "prev $?"
(exit 6); echo "$?" | cat
set -o pipefail
false | true; echo $?
true | (exit 3) | true; echo $?
(exit 2) | (exit 3) | true; echo $?
true | true; echo $?
set +o pipefail
f() { return 7; }
f | f; echo $?
x=$(false | true); echo "c=$?"
x=$(true | false); echo "c=$?"
if false | true; then echo then; fi
while true | false; do echo never; done; echo "w=$?"
set -e
false | true
echo "survived set -e"
true | false || echo "or branch"
