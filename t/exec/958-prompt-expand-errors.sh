# ${x@P} keeps the decoded prompt when its expansion fails, reporting
# the error and going on, even for errors that would end the shell. Its
# command substitutions change neither $? nor an assignment's status.
# Under set -e an unbound variable or ${x?} still ends the shell.
p='\101\\|\$|$(echo hi)|${x?boom}'
v=${p@P}; echo "st=$? [$v]"
echo "[${p@P}]"
p='$((1/0)) ok'; echo "[${p@P}]"
p='${y:?}'; echo "[${p@P}] $?"
(set -u; p='$nope'; echo "[${p@P}]"); echo "sub $?"
p='$(exit 3)'
v=${p@P}; echo "st $?"
false; v=${p@P}; echo "st $?"
v=$(exit 4)${p@P}; echo "st $?"
false; echo "${p@P} $?"
p='$(exit 5)$?'; echo "[${p@P}] $?"
p='$((1/0))'; (set -e; echo "[${p@P}]"; echo arith goes on)
trap 'echo "exit $?"' EXIT
set -e
p='${x:?no}'
if true "${p@P}"; then echo then; fi
echo not reached
