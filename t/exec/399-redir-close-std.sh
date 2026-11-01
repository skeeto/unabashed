# closing and restoring the standard descriptors
exec 3>&1
exec 1>&-
echo lost
exec 1>&3 3>&-
echo "back status $?"
exec 4>&2 2>&-
cd /nonexistent
exec 2>&4 4>&-
echo "status $?"
{ echo "a"; } 1>&- 2>&-
echo "after group $?"
read x 0<&- <<<"redir after close"
echo "x=$x"
