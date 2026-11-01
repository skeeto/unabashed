# unknown and malformed transformations
x=abc
( echo "${x@Z}"; echo after ); echo "status $?"
( echo "${x@}"; echo after ); echo "status $?"
( echo "${x@QQ}"; echo after ); echo "status $?"
( echo "${x@q}"; echo after ); echo "status $?"
echo end
