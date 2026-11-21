# The line bash is at when its EXIT action runs, as the messages it
# prints outside the action show (a readonly BASH_TRAPSIG fails before
# and after it). Each parse_and_execute bash leaves on the way (-c,
# eval, source, a command substitution) puts back the line it began at,
# so under -c there is none; a script's reader keeps its line, past the
# end at the end of input, or that of the command that exited; set -e
# acts once the line went back to where the enclosing for, case, call
# or top-level command left it; a subshell ends at its own line. bash's
# report_error runs the action at once, at the current line. A trap
# action's syntax error names the kind of trap.
r='readonly BASH_TRAPSIG=x; trap : EXIT'
run() {
    printf '%s\n' "$r" "$@" >s.sh
    echo "-- $*"
    bash ./s.sh 2>&1
    bash -c "$(cat s.sh)" 2>&1
}
run 'echo one' 'true'
run 'echo one' 'true' '' ''
printf '%s\necho one\ntrue # no newline' "$r" >s.sh
bash ./s.sh 2>&1
run 'cat <<E' 'x' 'E'
run 'f() {' ':' '}' 'f'
run 'if true; then' 'exit' 'fi'
run 'eval "' '' 'exit 4"' 'echo no'
run 'f() {' ' eval "' 'exit"' '}' 'f'
run 'set -e' 'while true; do' 'false' 'done' ''
run 'set -e' 'f() {' 'if true; then' 'false' 'fi' '}' '' 'f'
run 'set -e' 'for i in 1; do' ':' 'false' 'done'
run 'set -e' 'case x in' 'x)' 'false' 'esac'
run 'set -e' 'f()' '{' ':' 'false' '}' 'f'
run 'set -e' 'x=$(' 'exit 3' ')'
run 'eval "' 'set -e' 'false"'
printf '\n\n\nexit 3\n' >src.sh
run '' '. ./src.sh'
run 'x=$( trap : EXIT' 'true' ')'
run '( trap : EXIT' 'true' ')'
run '( trap : EXIT' 'exit 2' ')'
run 'set -e; readonly q' '' 'if read q <<<x; then :; fi'
# Out of a function, such a parse leaves a file exit was sourced in
printf 'set -- s1 s2\nexit 3\n' >src.sh
for c in '. ./src.sh' 'eval ". ./src.sh"' 'f() { . ./src.sh; }; f a'; do
    printf '%s\n' "trap 'echo \"[\${FUNCNAME[*]}] [\${BASH_SOURCE[*]}] [\$*]\"' EXIT" "set -- top" "$c" >s.sh
    bash ./s.sh; bash -c "$(cat s.sh)"
done
bash -c 'trap "if" USR1; kill -USR1 $$; trap "if" EXIT; trap "if" ERR; false
f() { :; }; trap "if" RETURN; set -T; f; trap - RETURN; trap "fi" DEBUG; :' 2>&1
rm -f s.sh src.sh
