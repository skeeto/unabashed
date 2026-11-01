t() { "$@"; echo "$? <- $*"; }
t test ! ""
t test ! x
t test ! ! x
t test ! ! ""
t test ! -n x
t test ! -z x
t test ! a = b
t test ! a = a
t test x -a y
t test x -a ""
t test "" -o y
t test "" -o ""
t test -n x -a -n y
t test -n x -a -z y
t test -z x -o -n y
t test 1 -eq 1 -a 2 -eq 2 -a 3 -eq 3
t test 1 -eq 1 -a 2 -eq 3 -o 4 -eq 4
t test 1 -eq 2 -o 2 -eq 2 -a 3 -eq 4
t test ! 1 -eq 2 -a ! 2 -eq 3
t test ! "" -a ! ""
t test x -a y -a z -a ""
t test "" -o "" -o "" -o z
t [ a = a -a b = b ]
t [ a = b -o b = b ]
