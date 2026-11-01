# GLOBSORT by size, mtime and atime, with names breaking ties in the
# same direction.
cd /tmp && mkdir gt && cd gt
printf 1234 > big; printf 12 > mid; printf 1 > small; : > empty; : > alsoempty
touch -d @1000 mid; touch -d @3000 small; touch -d @2000 big
touch -d @500 empty alsoempty
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
GLOBSORT=size;   show size *
GLOBSORT=-size;  show -size *
GLOBSORT=mtime;  show mtime *
GLOBSORT=-mtime; show -mtime *
GLOBSORT=atime;  show atime *
GLOBSORT=-atime; show -atime *
GLOBSORT=blocks; show blocks *
ln -s nowhere dangling
GLOBSORT=size;   show 'size dangling' *
GLOBSORT=-mtime; show '-mtime dangling' *
