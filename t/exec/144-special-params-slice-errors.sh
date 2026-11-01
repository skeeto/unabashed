# errors slicing positional parameters
set -- a b c
( echo "${@:1:-1}"; echo after ); echo "status $?"
( echo "${*:0:-2}"; echo after ); echo "status $?"
( echo "${@:1/0}"; echo after ); echo "status $?"
echo end
