# A login shell reads ~/.bash_logout when the exit builtin ends it (or
# an interactive one ends), before the EXIT trap; not from a subshell,
# not at the end of a -c string, not for a non-login shell
mkdir h && HOME=$PWD/h && export HOME
echo 'echo "logout $? $x"' >h/.bash_logout
bash -l -c 'trap "echo trap \$?" EXIT; x=1; exit 5'
echo "status $?"
bash -l -c 'echo end of string'
bash -l -c '(exit 2); echo "after subshell $?"'
bash -l -c 'f() { exit 3; }; x=f; f'
echo "status $?"
bash -c 'exit 6'
echo "status $?"
echo 'echo "logout2"; exit 9' >h/.bash_logout
bash -l -c 'exit 7'
echo "status $?"
echo 'echo "logout3"; false' >h/.bash_logout
bash -l -c 'exit'
echo "status $?"
bash -l -c 'logout 8'
echo "status $?"
bash -c 'logout 8'
echo "status $?"
echo 'trap "echo trap from logout" EXIT' >h/.bash_logout
bash -l -c 'exit 1'
echo "status $?"
echo 'echo logout4' >h/.bash_logout
(exec -a -bash bash -c 'exit 2')
echo "status $?"
bash --noprofile -l -c 'exit 3'
echo "status $?"
