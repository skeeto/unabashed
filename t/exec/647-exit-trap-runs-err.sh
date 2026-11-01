# an EXIT action that fails runs the ERR trap, in a subshell and in
# the shell itself
( trap 'nosuchcmd1; echo "in exit $?"' EXIT ERR ); echo "st $?"
( trap 'echo exit; false' EXIT; trap 'echo err $?' ERR; true ); echo "st $?"
trap 'nosuchcmd2' EXIT ERR
echo end
