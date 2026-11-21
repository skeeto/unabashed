# Under set -T, as without it, bash runs DEBUG for each simple stage of
# a pipeline run with & in the shell, before it forks for the stage, so
# before the next command's; the job runs it for what a function or
# compound stage runs (logged apart, as it runs alongside), but not
# again for the stages. lastpipe does not apply to a job.
trap 'if ((BASH_SUBSHELL)); then echo "  job [$BASH_COMMAND] $BASH_SUBSHELL" >>log
else echo "dbg [$BASH_COMMAND]"; fi' DEBUG
set -T
echo a | cat >/dev/null &
wait
true | false | cat </dev/null &
wait
{ true; } | cat >/dev/null &
wait
f() { :; }
f | cat &
wait
true | f >/dev/null &
wait
shopt -s lastpipe
true | echo "lastpipe $BASH_SUBSHELL" >>log &
wait
set +T
echo a | f &
wait
trap - DEBUG
cat log
rm log
