# Errors bash reports with report_error (a bad substitution, subscript
# or indirection, an unbound array element, `cannot assign', failglob's
# no match, a list assigned to an element) end the shell under set -e,
# in a condition or an && list too, with $? or else 1 (a bad
# substitution, `cannot assign' and no match set 1 first). Arithmetic
# errors and `substring expression < 0' do not, nor anything without
# set -e, nor eval or source run as a condition.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "set -e; (exit 3) || { $c; }
echo after" 2>&1; echo "status $?"
done <<'EOF'
echo ${x!}
u=; echo ${!u}
u=1a; echo ${!u}
echo ${!nosuch}
x=(1); echo ${x[-5]}
x=(1); echo ${#x[-5]}
echo ${arr[-1]:=a}
echo ${1=x}
shopt -s failglob; echo /nope*
a[]=1
x=1 a[]=1
a=([]=1)
x=(1); x[-5]+=1
declare a[]=1
declare -A m; m[]=1
a=(1); a[0]=(2)
echo $(( a[] ))
set -u; echo ${a[@]}${a[1]}
echo `echo a\`
echo ${x!} && echo and
if echo ${x!}; then :; fi
( echo ${x!}; echo in ); echo "sub $?"
y=$(echo ${x!}; echo in); echo "comsub <$y>"
y=$(set -e; echo ${x!}; echo in); echo "comsub -e <$y>"
if eval 'echo ${x!}'; then :; fi
echo $((1/0))
x=abc; echo ${x:1:-5}
set +e; echo ${x!}
EOF

# Not set -e, but the status an error leaves at exit: an EXIT trap
# ended by anything but exit or set -e (a text abandoned, as for a
# subscript's arithmetic error or exit's too many arguments) leaves
# the status the trap began with, and a return in a function's trap
# the return's, as bash's run_exit_trap.
while IFS= read -r c; do
    bash -c "$c" 2>&1; echo "status $?"
done <<'EOF'
trap 'map[a b]=1; echo after' EXIT
trap 'x[1/0]=1; echo after' EXIT; exit 3
trap 'declare -i n; n=1/0; echo after' EXIT; exit 3
trap 'exit 1 2; echo after' EXIT; exit 3
trap 'eval "map[a b]=1"; echo after' EXIT
trap 'set -e; false; echo after' EXIT; exit 3
trap 'exit 5' EXIT; exit 3
f() { trap 'return 6; echo after' EXIT; exit 3; }; f; echo no
f() { trap 'return' EXIT; exit 3; }; f
EOF

# Nor this: eval and source leave $? as it was for their text, and
# give 0 only if it runs nothing, as bash's parse_and_execute's
# last_result.
while IFS= read -r c; do
    bash -c "$c" 2>&1; echo "status $?"
done <<'EOF'
false; eval 'echo $?'; echo $?
false; eval '# c'; echo $?
false; eval $'\n\necho $?'
false; eval 'if'; echo $?
false; . /dev/null; echo $?
false; . /dev/stdin <<<'echo $?'; echo $?
false; . /dev/stdin <<<'return'; echo $?
f() { false; eval 'return'; }; f; echo $?
(exit 3); eval 'eval "echo \$?"'
EOF
