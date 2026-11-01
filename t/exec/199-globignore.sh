# GLOBIGNORE removes matching names from glob results, and setting it
# makes * match dot files.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > a.c; : > b.c; : > a.h; : > .hc
GLOBIGNORE='*.h'
show *
show a*
GLOBIGNORE='a*:.hc'
show *
GLOBIGNORE='*'
show *
show *.c
GLOBIGNORE=
show *
unset GLOBIGNORE
show *
