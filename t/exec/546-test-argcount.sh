t() { "$@"; echo "$? <- $*"; }
t test
t test -n
t test -z
t test !
t test '('
t test -a
t test -o
t test -x
t test ! !
t test ! -n
t test ! -z
t test -n -n
t test -z -z
t test = =
t test x =
t test = x
t test ! = !
t test -a -a -a
t test -o -o -o
t test ! ! !
t test ! -z -z
t test -n = -n
t test = = = =
t test ! ! ! !
t test ! x = y
t test '(' -n ')'
t test ! '(' x ')'
t test a b c d e
