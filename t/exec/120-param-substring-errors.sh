# substring errors: negative length, bad arithmetic
x=abcdef
( echo "${x:4:-3}"; echo after ); echo "status $?"
n=-5; ( echo "${x:1:n}"; echo after ); echo "status $?"
( echo "${x:1:$n}"; echo after ); echo "status $?"
( echo "${x:1/0}"; echo after ); echo "status $?"
( echo "${x:1:2+}"; echo after ); echo "status $?"
( echo "${x:i<3?5:1}"; echo after ); echo "status $?"
( echo "${x:(i<3?1:2)}"; echo after ); echo "status $?"
( echo "${x: a b}"; echo after ); echo "status $?"
echo end
