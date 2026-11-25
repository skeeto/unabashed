# A trap action is parsed anew, so ERR and set -e work in it even when
# it runs inside a condition, unless eval, source or command run as the
# condition turned both off; DEBUG, ERR and RETURN actions that end the
# shell do not run again during its EXIT trap
(trap 'echo err' ERR; trap 'false; echo x' EXIT; exit 2 && :); echo "s=$?"
(trap 'echo err' ERR; trap 'false; echo x' EXIT; if exit 2; then :; fi); echo "s=$?"
(trap 'echo err' ERR; trap 'false; echo x' USR1; kill -USR1 $BASHPID && :; echo "s=$?")
(trap 'echo err' ERR; trap 'false; echo x' DEBUG; : && :)
(trap 'echo err' ERR; f() { :; }; trap 'false; echo x' RETURN; f && :)
(set -e; trap 'false; echo x' USR1; kill -USR1 $BASHPID && :; echo "s=$?"); echo "st=$?"
(trap 'echo err' ERR; trap 'false; echo x' USR1; if eval 'kill -USR1 $BASHPID'; then :; fi)
(set -e; trap 'false; echo x' EXIT; if eval 'exit 3'; then :; fi); echo "st=$?"
(set -e; trap 'false; echo x' EXIT; eval 'if eval "exit 3"; then :; fi'); echo "st=$?"
x=$(set -e; trap 'false; echo x' EXIT; if eval 'exit 3'; then :; fi); echo "st=$? [$x]"
echo ---
(trap 'echo exit; false; echo e2' EXIT; trap 'echo err; exit 3' ERR; false); echo "s=$?"
(trap 'echo exit; :; echo e2' EXIT; trap 'echo dbg; exit 4' DEBUG; :); echo "s=$?"
(f() { :; }; trap 'echo exit; f; echo e2' EXIT; trap 'echo ret; exit 3' RETURN; set -T; f); echo "s=$?"
(trap 'echo exit; false; echo e2' EXIT; trap 'echo err; : ${u?x}' ERR; false); echo "s=$?"
