# vos
# hash -p, and a script name that shadows a program of the same base
hash -p /usr/bin/printf.exe pf
type pf
hash -t pf
printf '#!/bin/sh\necho script\n' >argv
PATH=.:$PATH
type argv
hash -r
command -v argv
./argv
