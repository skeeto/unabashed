# Setting GLOBIGNORE to a non-empty value turns dotglob on; setting it
# empty leaves dotglob alone; unsetting it turns dotglob off. Names that
# GLOBIGNORE removes are matched against the whole path.
show() { printf '%s:' "$#"; printf '<%s>' "$@"; echo; }
cd /tmp
: > .d; : > e.c; : > e.o
shopt dotglob
GLOBIGNORE='*.o'
shopt dotglob
show *
show /tmp/*
GLOBIGNORE='/tmp/e.*'
show /tmp/*
show *
GLOBIGNORE=
shopt dotglob
show *
unset GLOBIGNORE
shopt dotglob
show *
shopt -s dotglob
GLOBIGNORE=.d
show *
show .*
unset GLOBIGNORE
show *
