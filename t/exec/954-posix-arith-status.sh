# In POSIX mode an arithmetic error ends a non-interactive shell with
# 127, but a ( ), comsub or compound command run apart (a stage, a job,
# a coprocess, a script without #!) with 1; a simple command run apart
# ends as the level around it would
set -o posix
(x=$((1/0)); echo no); echo "subshell $?"
y=$(x=$((1/0)); echo no); echo "comsub $?"
x=$((1/0)) | cat; echo "simple stage ${PIPESTATUS[0]}"
f() { x=$((1/0)); }; f | cat; echo "function stage ${PIPESTATUS[0]}"
(x=$((1/0))) | cat; echo "subshell stage ${PIPESTATUS[0]}"
{ x=$((1/0)); } | cat; echo "group stage ${PIPESTATUS[0]}"
if x=$((1/0)); then :; fi | cat; echo "if stage ${PIPESTATUS[0]}"
cat </dev/null | { x=$((1/0)); }; echo "last group stage ${PIPESTATUS[1]}"
cat </dev/null | x=$((1/0)); echo "last simple stage ${PIPESTATUS[1]}"
echo $((1/0)) & wait $!; echo "simple job $?"
{ x=$((1/0)); } & wait $!; echo "group job $?"
coproc { x=$((1/0)); }; wait $!; echo "coprocess $?"
( x=$((1/0)) | cat; echo "simple stage in a subshell ${PIPESTATUS[0]}" )
( x=$((1/0)) & wait $!; echo "simple job in a subshell $?" )
( (x=$((1/0))); echo "nested subshell $?" )
g() (echo $((1/0))); g; echo "subshell function $?"
( eval 'x=$((1/0))' ); echo "eval in a subshell $?"
( : >$((1/0)); echo no ); echo "redirection in a subshell $?"
( trap 'echo "EXIT trap $?"' EXIT; x=$((1/0)) ); echo "with a trap $?"
command_not_found_handle() { x=$((1/0)); }; nosuchcmd; echo "handler $?"
unset -f command_not_found_handle
printf 'x=$((1/0)); echo no\n' >noshebang; chmod +x noshebang
(./noshebang); echo "script $?"
trap 'echo "EXIT trap $?"' EXIT
x=$((1/0)); echo no
