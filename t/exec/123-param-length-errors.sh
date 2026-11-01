# bad ${#...} forms
( echo "${#x:-a}"; echo after ); echo "status $?"
( echo "${#x#a}"; echo after ); echo "status $?"
( echo "${#!}"; echo after ); echo "status $?"
echo end
