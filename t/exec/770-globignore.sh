# GLOBIGNORE: colon-separated patterns matched against whole paths;
# colons inside brackets and extglob groups do not separate them.
cd /tmp && mkdir gi && cd gi
touch a.c b.c c.o 'x:y' ';' '++' .hid
mkdir sub
touch sub/s.c sub/t.o
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
GLOBIGNORE='*.o'
show '*'       *
show 'sub/*'   sub/*
GLOBIGNORE='sub/*.o:*.c'
show '*'       *
show 'sub/*'   sub/*
GLOBIGNORE='*[:]*'
show 'colon in bracket' *
GLOBIGNORE='x[:]y:*.c'
show 'two' *
shopt -s extglob
GLOBIGNORE='+([^[:alnum:]]):*.[co]'
show 'extglob' *
GLOBIGNORE='@(x:y|sub)'
show 'group' *
shopt -u extglob
GLOBIGNORE='*'
show 'all' *
GLOBIGNORE=':'
show 'empty patterns' *
GLOBIGNORE='.*'
show 'dots' .* *
shopt -s nocaseglob
GLOBIGNORE='*.C'
show 'nocase' *
