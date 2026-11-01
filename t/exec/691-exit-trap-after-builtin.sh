# An EXIT trap that runs after a builtin escaped (exit, return, an
# error) writes its own output, not into the builtin's buffer
(trap 'T' EXIT; exit); echo "status $?"
(trap 'echo trap; nosuch' EXIT; printf x; exit 3); echo "status $?"
(trap 'declare -p v' EXIT; v=1; set -u; : $undefined); echo "status $?"
f() { trap 'echo in trap; cd /nonexistent' EXIT; return 4; }
(f; echo after); echo "status $?"
