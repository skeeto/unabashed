# An asynchronous simple command's words expand in bash's child before
# it redirects standard input from /dev/null, so a process
# substitution in them (or in an assignment) reads the shell's input,
# in a ( list ) too; the command itself, a function's body, and a
# compound command's procsubs read /dev/null. Under job control
# nothing is redirected.
printf 'l%s\n' 1 2 3 4 5 6 7 8 9 10 11 12 >f
exec <f
cat <(read x; echo "1 $x") & wait
cat & wait
echo "2 $(read y; echo $y)"
f() { cat "$1"; cat; }; f <(read x; echo "3 $x") & wait
cat <(read x; echo "4 $x") | cat & wait
(cat <(read x; echo "5 $x")) & wait
{ cat <(read x; echo "6 $x"); } & wait
( cat <(read x; echo "7 $x") & wait )
x=<(read x; echo "8 $x") cat & wait
read z; echo "9 $z"
echo x | { cat <(read x; echo "10 $x") & wait; }
set -m
cat <(read x; echo "11 $x") & wait
{ read x; echo "12 $x"; } & wait
read z; echo "13 $z"
