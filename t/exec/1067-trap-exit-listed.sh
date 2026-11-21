# The EXIT action stays listed while it runs, as the other traps' do in
# theirs: trap, trap -p, trap -p EXIT, and in a command substitution or
# a subshell, which list the parent's table. Reset in it, it is gone; set
# anew, the new action is listed but does not run. A ( ) or & subshell
# in it runs its own EXIT action; a command or process substitution, as
# in bash, does not.
bash -c 'trap "echo u" USR1; trap "trap; echo --; trap -p EXIT; trap -p USR1 EXIT" EXIT'
bash -c 'trap "echo \"[\$(trap -p)]\"; (trap -p EXIT)" EXIT'
bash -c 'trap "trap - EXIT; trap -p; echo reset" EXIT'
bash -c 'trap "trap \"echo second\" EXIT; trap -p EXIT" EXIT; exit 3'; echo "status $?"
bash -c 'f() { trap -p EXIT; }; trap f EXIT; g() { exit 2; }; g'; echo "status $?"
bash -c 'trap "echo err; trap -p ERR" ERR; false'
bash -c 'trap "echo debug; trap -p DEBUG" DEBUG; :'
bash -c 'trap "echo usr1; trap -p USR1" USR1; kill -USR1 $$'
bash -c 'trap "trap -p EXIT" EXIT; trap "echo term" TERM; kill $$'
echo "-- subshells in it"
bash -c 'trap "(trap \"echo sub\" EXIT; :); { trap \"echo bg\" EXIT; } & wait
  echo \"[\$(trap \"echo cs\" EXIT; :)]\"; cat <(trap \"echo ps\" EXIT; :)" EXIT'
bash -c 'echo "[$(trap "echo cs" EXIT; :)]"'
echo done
