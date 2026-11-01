t() { "$@"; echo "$? <- $*"; }
t test '(' x ')'
t test '(' "" ')'
t test '(' -n x ')'
t test '(' -z x ')'
t test '(' a = a ')'
t test '(' a = b ')'
t test ! '(' a = b ')'
t test '(' a = b ')' -o '(' c = c ')'
t test '(' "" -o x ')' -a '(' y ')'
t test '(' '(' x ')' ')'
t test '(' '(' '(' a = a ')' ')' ')'
t test '(' -n x -a -z "" ')' -a x
t [ '(' 1 -eq 1 ')' ]
t [ ! '(' ! a ')' ]
t test '(' ')'
t test '(' '(' ')'
t test ')' 
t test '(' 
