# An unknown @ transform of a set value (${x@x}, ${x@}, ${x@Qx}) is a
# fatal error, as an unset ${x?} is: a non-interactive shell ends with
# status 127, wherever it is (a function, eval, source, [[ ]]), and a
# ( ) subshell or command substitution ends itself only. Other bad
# substitutions (${x&}) only abandon the command. An unset value is not
# checked. A fatal error in the EXIT trap exits with the status the
# trap began with.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "x=abc; $c; echo after" 2>&1; echo "status $?"
done <<'EOF'
echo ${x@x}
echo ${x@}
echo ${x@Qx}
echo "${x@Z}"
a=(1 2); echo ${a[@]@x}
a=(); echo ${a[@]@x}
echo ${u@x} ${u[@]@x}
declare -A m=([k]=v); echo ${m[k]@x}
set -- a; echo ${@@x}
echo ${x&}
f() { echo ${x@x}; echo in; }; f
eval 'echo ${x@x}; echo in'
echo 'echo ${x@x}; echo in' >src.sh; . ./src.sh
[[ ${x@x} ]]
( echo ${x@x}; echo in ); echo "sub $?"
y=$(echo ${x@x}; echo in); echo "comsub $? <$y>"
echo ${x@x} | cat; echo "pipe $?"
set -e; echo ${x@x}
set -o posix; echo ${x&}
trap 'echo ${x@x}; echo intrap' EXIT; exit 4
trap 'echo ${u?}; echo intrap' EXIT; exit 5
trap 'echo ${x&}; echo intrap' EXIT; exit 6
EOF
rm -f src.sh
