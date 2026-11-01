t() { if eval "$1"; then echo "T: $1"; else echo "F: $1"; fi; }
e=
t '[[ -e $e ]]'
t '[[ -f $e ]]'
t '[[ -d $e ]]'
t '[[ -r "" ]]'
t '[[ -o errexit ]]'
t '[[ -o nosuch ]]'
t '[[ ! -o nosuch ]]'
set -u
t '[[ -o nounset ]]'
set +u
t '[[ -n -n ]]'
t '[[ -z -z ]]'
t '[[ -n == -n ]]'
t '[[ -e /tmp && -d /tmp && ! -f /tmp ]]'
t '[[ -s /tmp/nope || ! -s /tmp/nope ]]'
