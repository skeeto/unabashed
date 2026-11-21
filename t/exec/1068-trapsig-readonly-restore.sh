# A readonly BASH_TRAPSIG fails twice per trap action, as in bash: once
# when the action starts and once when it ends, as each tries to set it,
# for a signal, ERR, DEBUG, RETURN, one that returns from a function,
# and EXIT, even after exit. Under set -e, as for bash's other reported
# errors, the first failure ends the shell, with $? or else 1, before
# the action and with no EXIT action; set -e in the action ends it at
# the second.
r='readonly BASH_TRAPSIG=x;'
bash -c "$r"' trap "echo usr1" USR1; kill -USR1 $$; echo after' 2>&1
bash -c "$r"' trap "echo err" ERR; false; echo after' 2>&1
bash -c "$r"' trap "echo debug" DEBUG; :' 2>&1
bash -c "$r"' f() { :; }; trap "echo ret" RETURN; set -T; f; echo after' 2>&1
bash -c "$r"' f() { kill -USR1 $$; echo no; }; trap "echo usr1; return 3" USR1; f; echo "after $?"' 2>&1
printf '%s\n' "$r" 'trap "echo exit" EXIT' 'f() {' '  exit 4' '}' 'f' >trapsig.sh
bash ./trapsig.sh 2>&1; echo "status $?"
echo "-- set -e"
r="$r"' set -e; trap "echo exit" EXIT;'
bash -c "$r"' trap "echo usr1" USR1; kill -USR1 $$; echo no' 2>&1; echo "status $?"
bash -c "$r"' trap "echo err" ERR; f() { return 3; }; f; echo no' 2>&1; echo "status $?"
bash -c "$r"' trap "echo debug" DEBUG; echo no' 2>&1; echo "status $?"
bash -c "$r"' trap "echo usr1" USR1; if kill -USR1 $$; then echo no; fi' 2>&1; echo "status $?"
bash -c 'readonly BASH_TRAPSIG=x; trap "set -e; echo usr1" USR1; kill -USR1 $$; echo no' 2>&1; echo "status $?"
bash -c 'readonly BASH_TRAPSIG=x; trap "set -e; echo usr1; (exit 5)" USR1; kill -USR1 $$; echo no' 2>&1; echo "status $?"
printf '%s\n' 'readonly BASH_TRAPSIG=x' 'set -e' 'trap "echo exit" EXIT' 'exit 2' >trapsig.sh
bash ./trapsig.sh 2>&1; echo "status $?"
rm -f trapsig.sh
echo done
