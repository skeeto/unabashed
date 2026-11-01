t() { "$@"; echo "$? <- $*"; }
t test '(' = '('
t test ')' = ')'
t test '!' = '!'
t test -a = -a
t test -o = -o
t test -n = -n
t test '!' = x
t test '(' != ')'
t test ']' = ']'
t [ ']' = ']' ]
t [ '(' = '(' ]
t [ '!' ]
t [ '(' ]
t [ ')' ]
t [ -a ]
t test -e -a -e
t test ! -a !
