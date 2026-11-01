# GLOBSORT names the sort order of pathname expansion: name, numeric,
# nosort, with - to reverse; unknown values sort by name.
cd /tmp && mkdir go && cd go
touch 10 9 100 b a B 1a 007 7
show() { printf '%s:' "$1"; shift; printf ' %s' "$@"; echo; }
show default *
GLOBSORT=name;     show name *
GLOBSORT=-name;    show -name *
GLOBSORT=+name;    show +name *
GLOBSORT=;         show empty *
GLOBSORT=+;        show + *
GLOBSORT=-;        show - *
GLOBSORT=numeric;  show numeric *
GLOBSORT=-numeric; show -numeric *
GLOBSORT=bogus;    show bogus *
GLOBSORT=-bogus;   show -bogus *
GLOBSORT=' -name'; show ' -name' *
GLOBSORT='name ';  show 'name ' *
GLOBSORT=nosort;   show nosort 1*
unset GLOBSORT;    show unset *
mkdir d; touch d/2 d/11
GLOBSORT=numeric;  show 'numeric paths' d/* *
