# $_ is the last word of the previous simple command, bound after it
# runs (so not yet inside a function it calls), empty after assignments
# alone, and not exported
echo x y
echo "[$_]"
f() { echo "in: $_"; }
f arg1 arg2
echo "after: $_"
a=1
echo "assign: [$_]"
_=foo
echo "set: [$_]"
: ; declare -p _
unset _
echo "unset: ${_-unset}"
set -a; echo z; env | grep -c '^_=z' ; set +a
echo "${_+set}"
