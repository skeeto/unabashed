# exec without a command keeps its redirections when command runs it
# too (command -p, command command), and a failure undoes them all;
# command makes it no special builtin, so POSIX mode goes on
exec 2>&1
command exec 3>/nonexistent/x; echo "after $?"
(exec 3>/nonexistent/x; echo "sub after $?"); echo "sub=$?"
(set -o posix; command exec 3>/nonexistent/x; echo "posix after $?"); echo "p=$?"
(set -o posix; exec 3>/nonexistent/x; echo "posix plain $?"); echo "pp=$?"
command exec 4>/tmp/ok 5</nonexistent/y; echo "s=$?"; echo x >&4 && echo "fd4 kept"
command exec 3>/tmp/ce; echo three >&3; cat /tmp/ce
command command exec 4>/tmp/ce4; echo four >&4; cat /tmp/ce4
builtin exec 5>/tmp/ce5 2>/dev/null; echo "s=$?"
command -p exec 6>/tmp/ce6; echo six >&6; cat /tmp/ce6
x=1 command exec 7>/tmp/ce7; echo seven >&7; cat /tmp/ce7; echo "x=$x"
