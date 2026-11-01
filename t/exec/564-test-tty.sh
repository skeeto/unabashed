t() { "$@"; echo "$? <- $*"; }
t test -t 0
t test -t 1
t test -t 2
t test -t 99
t test -t
t test -t x
t test -t -1
t test -t 0 -a x
t test -t -a x
t test -t -o x
t [[ -t 0 ]]
: > /tmp/f
t test -O /tmp/f
t test -G /tmp/f
t test -N /tmp/f
t test -u /tmp/f
t test -k /tmp
t test -c /dev/null
t test -O /
t test -w /
t test -w /home/oracle
t test -e /dev/stdin
t test -e /dev/fd/0
t test -e /dev/fd/7
