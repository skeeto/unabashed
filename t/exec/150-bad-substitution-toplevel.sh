# bad substitution aborts the current command, not the shell
echo before
echo "${x;y}"
echo "status $?"
x=1; echo "${x:?}" "${x+ok}"
echo "${x/}" "${#x}"
