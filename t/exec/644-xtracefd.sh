# BASH_XTRACEFD: traces go to the descriptor it names; an invalid value
# is reported and traces stay on stderr; unsetting it restores stderr
BASH_XTRACEFD=7
exec 7>/tmp/trace.$$
BASH_XTRACEFD=7
set -x
echo one
: two
set +x
unset BASH_XTRACEFD
set -x
echo three
set +x
exec 7>&-
echo "trace file:"
cat /tmp/trace.$$
rm -f /tmp/trace.$$
