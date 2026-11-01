# invalid indirect expansions
unset u
( echo "[${!u}]"; echo after ); echo "status $?"
bad='a b'; ( echo "${!bad}"; echo after ); echo "status $?"
bad2='x-y'; ( echo "${!bad2}"; echo after ); echo "status $?"
emp=; ( echo "[${!emp}]"; echo after ); echo "status $?"
( echo "${!u:-def}"; echo after ); echo "status $?"
op='x:-y'; ( echo "${!op}"; echo after ); echo "status $?"
echo end
