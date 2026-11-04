# A PATH assigned before a program is the one searched, without the
# remembered paths, and command_not_found_handle sees every temporary
# assignment
PATH=/nowhere cat </dev/null; echo "1 $?"
PATH= cat </dev/null; echo "2 $?"
PATH=/nowhere:$PATH cat </dev/null; echo "3 $?"
hash cat; PATH=$PATH cat </dev/null; hash; echo "4 $?"
declare -n r=PATH
r=/nowhere cat </dev/null; echo "5 $?"
unset -n r
true | PATH=/nowhere cat | cat; echo "6 $? $PATH"
command_not_found_handle() { echo "h $1 x=$x p=$PATH"; }
x=1 nosuch; echo "7 $?"
PATH=/nowhere x=2 cat; echo "8 $?"
true | PATH=/nowhere cat | cat; echo "9 $?"
unset -f command_not_found_handle
x=3 printenv x; echo "10 $? x=$x"
readonly x
x=4 cat </dev/null; echo "11 $?"
