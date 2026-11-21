# An error caught inside an arithmetic evaluation's callback (here a
# program's redirection in ${ cmd; } in a subscript) leaves the
# enclosing evaluation's recursion limit in force: the recursion after
# it stops at the same depth as without the error. The subshells' EXIT
# traps show how far each got.
n=0
b[0]='b[$((n++, b[0]))]'
i=1
while ((i < 300)); do eval "v$i=v$((i+1))"; ((i++)); done
v300='x[${ cat </dev/null >${u?}; }$((b[0]))]'
(trap 'echo n=$n' EXIT; echo $(( v1 )) ); echo "status $?"
v300='x[$((b[0]))]'
(trap 'echo n=$n' EXIT; echo $(( v1 )) ); echo "status $?"
# caught outside any evaluation, the limit starts afresh
cat </dev/null >${u?}
(trap 'echo n=$n' EXIT; echo $(( x[ $((b[0])) ] )) ); echo "status $?"
