# ${x?w} and ${x:?w} messages and status
unset u; e=; s=ok
echo "${s?no} ${s:?no} ${e?no}"
( echo "${u?}" ); echo "status $?"
( echo "${u:?}" ); echo "status $?"
( echo "${e:?}" ); echo "status $?"
( echo "${u?custom message}" ); echo "status $?"
( echo "${e:?is $s empty}" ); echo "status $?"
( echo "${1?}" ); echo "status $?"
( echo "${1:?need arg}" ); echo "status $?"
( : "${u:?}"; echo not reached ); echo "status $?"
f() { echo "${1:?f needs arg}"; echo after; }
( f ); echo "status $?"
x=$(echo "${u?in cmdsub}"; echo still); echo "status $? [$x]"
echo end
