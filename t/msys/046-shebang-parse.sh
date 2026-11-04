# Shebang lines: blanks, CRLF, the rest of the line as one argument,
# missing interpreters
printf '#! /bin/sh\necho spaced\n' >a
printf '#!/bin/sh   \necho trailing\n' >b
printf '#!/bin/sh\r\necho crlf\r\n' >c
printf '#!/bin/echo one two\n' >d
printf '#!/bin/echo\n' >e
printf '#!/no/such/interpreter\necho never\n' >f
printf '#!\necho empty\n' >g
for x in a b c d e; do ./$x arg; done
./f; echo "status $?"
./g; echo "status $?"
