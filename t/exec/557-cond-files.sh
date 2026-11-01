t() { if eval "$1"; then echo "T: $1"; else echo "F: $1"; fi; }
: > /tmp/e
echo x > /tmp/s
t '[[ -e /tmp/e ]]'
t '[[ -e /tmp/none ]]'
t '[[ -f /tmp/e ]]'
t '[[ -f /tmp ]]'
t '[[ -d /tmp ]]'
t '[[ -d /tmp/e ]]'
t '[[ -s /tmp/e ]]'
t '[[ -s /tmp/s ]]'
t '[[ -r /tmp/s && -w /tmp/s ]]'
t '[[ -x /tmp/s ]]'
t '[[ -x /tmp ]]'
t '[[ -L /tmp/s ]]'
t '[[ ! -e /tmp/none ]]'
t '[[ -e /tmp/none || -e /tmp/e ]]'
f=/tmp/s
t '[[ -s $f ]]'
t '[[ -f "$f" ]]'
cd /tmp
t '[[ -f s ]]'
t '[[ -d ~ ]]'
t '[[ -d ~/ ]]'
t '[[ -a /tmp/e ]]'
