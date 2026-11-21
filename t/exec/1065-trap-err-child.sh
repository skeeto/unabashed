# ERR and set -e in children: bash's forked child exits with the status
# of the whole command it runs (a simple command as a job or pipeline
# stage), so no ERR runs for it there, only for what it runs; a
# background pipeline's status is never looked at; a ( list ) as the
# last stage is waited for as one alone, so ERR runs for it, then again
# for the pipeline
set -E
trap 'echo "  err $LINENO $?"' ERR
f() { false; }
g() { return 3; }
echo "-- jobs"
false & wait
f & wait
g & wait
g >/dev/null & wait
x=$(false) & wait
x=1 </nonexistent & wait
{ false; } & wait
{ true; false; } & wait
( true; false ) & wait
true && false & wait
! true & wait
coproc g; wait
coproc { false; }; wait
echo "-- stages"
false | cat
cat </dev/null | false
f | cat
g | cat
g </nonexistent | cat
{ true; false; } | cat
true | { false; }
true | ( false )
true | ( exit 4 )
( false ) | true
! true | ( false )
true | ( false ) || :
echo "-- background pipelines"
g | g & wait
true | ( exit 4 ) & wait
{ false | true; } & wait
echo "-- pipefail"
set -o pipefail
false | cat
g | cat
( exit 5 ) | ( exit 6 ) | ( exit 0 )
g | g & wait
{ false | true; } & wait
set +o pipefail
echo "-- lastpipe"
shopt -s lastpipe
true | false
true | ( false )
shopt -u lastpipe
echo "-- errtrace off"
set +E
f | cat
true | { false; }
true | ( false )
(false)
echo "-- set -e"
set -e
h() { g; echo "  after g"; }
h & wait $! || echo "  status $?"
{ false; echo "  not reached"; } & wait $! || echo "  status $?"
h | cat
g | cat
( false; echo "  not reached" ) | cat
true | ( exit 7 ) || echo "  status $?"
echo done
