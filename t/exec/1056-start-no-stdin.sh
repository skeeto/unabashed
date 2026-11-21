# A shell started with fd 0 closed has it closed, not open and failing:
# dup and redirection from it fail as for any closed descriptor, reads
# and programs see EBADF, and it opens again like any other
bash -c '
exec 3<&0; echo "dup $?"
: <&0; echo "same $?"
read x; echo "read $?"
cat; echo "cat $?"
[ -t 0 ]; echo "tty $?"
[ -e /dev/fd/0 ]; echo "fd $?"
cat | cat; echo "pipeline ${PIPESTATUS[*]}"
(cat); echo "subshell $?"
cat <<<string; echo "herestring $?"
cat <<EOF
heredoc
EOF
echo "heredoc $?"
echo "cmdsub $(echo inner)"
cat </dev/null; echo "null $?"
exec </dev/null; cat; echo "reopened $?"
exec <&-; read x; echo "closed again $?"
' bash <&-
echo "status $?"
