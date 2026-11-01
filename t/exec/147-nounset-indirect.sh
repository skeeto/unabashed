# set -u with indirection
set -u
r=missing
( echo "${!r}"; echo after ); echo "status $?"
( echo "[${!r-def}]"; echo after ); echo "status $?"
( echo "${!unsetref}"; echo after ); echo "status $?"
target=t; r=target; echo "${!r}"
echo end
