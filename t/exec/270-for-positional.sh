# for over $@ and $* in various quoting
set -- 'a b' '' c
for x in "$@"; do echo "q@ [$x]"; done
for x in $@; do echo "u@ [$x]"; done
for x in "$*"; do echo "q* [$x]"; done
for x in $*; do echo "u* [$x]"; done
IFS=:
for x in "$*"; do echo "colon q* [$x]"; done
v='p:q::r'
for x in $v; do echo "colon split [$x]"; done
IFS=' '
for x in "$@"x; do echo "suffix [$x]"; done
for x in "${@}"; do echo "braced [$x]"; done
set --
for x in "$@"; do echo never; done
for x in "$@" ''; do echo "empty plus [$x]"; done
