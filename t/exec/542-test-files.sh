t() { "$@"; echo "$? <- $*"; }
: > /tmp/empty
echo data > /tmp/full
t test -e /tmp/empty
t test -e /tmp/nope
t test -a /tmp/full
t test -f /tmp/empty
t test -f /tmp
t test -d /tmp
t test -d /tmp/full
t test -s /tmp/empty
t test -s /tmp/full
t test -s /tmp/nope
t test -r /tmp/full
t test -w /tmp/full
t test -x /tmp/full
t test -x /tmp
t test -r /tmp/nope
t test -L /tmp/full
t test -h /tmp/full
t test -e ""
t test -d ""
t test -f /tmp/
t test -d /tmp/
t test -d /tmp/./
t test -e /tmp/full/
t test -e /tmp/full/x
cd /tmp
t test -f full
t test -d .
t test -d ..
t [ -f empty ]
t [ ! -f nope ]
