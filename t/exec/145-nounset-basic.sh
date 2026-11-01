# set -u with simple and braced expansions
set -u
e=
echo "[$e] [${e}] [${e-x}] [${u-x}] [${u:-y}] [${u+z}]"
( echo "$u"; echo after ); echo "status $?"
( echo "${u}"; echo after ); echo "status $?"
( echo "${#u}"; echo after ); echo "status $?"
( echo "${u#a}"; echo after ); echo "status $?"
( echo "${u/a/b}"; echo after ); echo "status $?"
( echo "${u^^}"; echo after ); echo "status $?"
( echo "${u:1}"; echo after ); echo "status $?"
( echo "${u@Q}"; echo after ); echo "status $?"
( x="a${u}b"; echo after ); echo "status $?"
echo end
