# A background job of a shell started with fd 1 or fd 2 closed has it
# closed too: a dup of it fails as in the shell itself. The job's copy
# of the shell once took the closed descriptor for an open one.
bash -c '
{ exec 4>&1; echo "job dup $?" >&2; echo x; echo "job echo $?" >&2; } & wait
exec 5>&1; echo "dup $?" >&2
' bash 2>&1 >&-
echo "status $?"
bash -c '
{ exec 4>&2; echo "job dup $?"; nosuch; echo "job missing $?"; } & wait
exec 5>&2; echo "dup $?"
' bash 2>&-
echo "status $?"
