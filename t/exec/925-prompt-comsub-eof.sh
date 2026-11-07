# An unterminated $( in a prompt string is reported and comes to
# nothing, and the command goes on; a bare $( ending the text is nothing
exec 2>&1
x='a$( b'; echo "[${x@P}]"; echo st=$?
x='bare $('; echo "[${x@P}]"
PS4='+$( '; set -x; : traced; set +x
PS4='t$('; set -x; : bare; set +x
PS4='c${nope:?bad} '; set -x; : raw; set +x
echo end
