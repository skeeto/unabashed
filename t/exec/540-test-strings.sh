t() { "$@"; echo "$? <- $*"; }
t test -z ""
t test -z x
t test -n ""
t test -n x
t test abc
t test ""
t test abc = abc
t test abc = abd
t test abc == abc
t test abc != abd
t test abc != abc
t test "" = ""
t test a '<' b
t test b '<' a
t test B '<' a
t test abc '>' abd
t test abcd '>' abc
t [ x = x ]
t [ x != x ]
t [ "a b" = "a b" ]
t [ '*' = '*' ]
t [ '*' = 'x' ]
t [ -n "" ]
t [ -z "" ]
t [ = ]
t [ -n ]
t [ -z ]
