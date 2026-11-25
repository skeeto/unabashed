# set -n takes effect at once, as bash checks it before every command
# it executes: nothing more runs, not the rest of a list, group, if,
# function, eval or sourced file, nor the EXIT trap, nor exit or set
# +n, and the status stays as it was. What follows is still parsed. A
# ( ) subshell or command substitution stops itself only.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "trap 'echo exit \$?' EXIT; $c; echo after" 2>&1; echo "status $?"
done <<'EOF'
set -n; echo $?
set -o noexec; echo a
{ set -n; echo in; }
f() { set -n; echo in; }; f
eval 'set -n; echo in'
echo 'set -n; echo in' >src.sh; . ./src.sh
if set -n; then echo then; fi
set -n && echo and
set -n; set +n
set -n; exit 3
false; set -n
for i in 1 2; do echo $i; set -n; done
trap 'set -n' USR1; kill -USR1 $$
x=abc; set -n; echo ${x@x}
( set -n; echo in ); echo "sub $?"
x=$(set -n; echo in); echo "comsub <$x>"
set -n | cat; echo "pipe $?"
EOF
rm -f src.sh
