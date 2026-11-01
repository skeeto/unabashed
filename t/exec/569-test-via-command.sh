t() { "$@"; echo "$? <- $*"; }
t command test a = a
t builtin test a = b
t builtin [ x ]
t command [ "" ]
b='['
t $b 1 -lt 2 ]
t eval '[ 1 -gt 2 ]'
test a = a > /tmp/out; echo "redir $?"
[ 1 = 1 ] 2>/tmp/err; echo "redir2 $?"
[ 1 = ] 2>/tmp/err; echo "redir3 $?"
while read -r line; do echo "err: $line"; done < /tmp/err
test 2>&1 x y; echo "redir4 $?"
type test [ [[ ((
