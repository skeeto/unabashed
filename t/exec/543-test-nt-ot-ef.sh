t() { "$@"; echo "$? <- $*"; }
: > /tmp/a
t test /tmp/a -nt /tmp/missing
t test /tmp/missing -nt /tmp/a
t test /tmp/a -ot /tmp/missing
t test /tmp/missing -ot /tmp/a
t test /tmp/missing -nt /tmp/missing2
t test /tmp/missing -ot /tmp/missing2
t test /tmp/a -nt /tmp/a
t test /tmp/a -ot /tmp/a
t test /tmp/a -ef /tmp/a
t test /tmp/a -ef /tmp/./a
t test /tmp/a -ef /tmp/missing
t test /tmp/missing -ef /tmp/missing
t test /tmp -ef /tmp/
cd /tmp
t test a -ef /tmp/a
t [ a -nt missing ]
t [[ a -nt missing ]]
t [[ missing -ot a ]]
t [[ a -ef ./a ]]
