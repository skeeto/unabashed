# printf %n, %q and %Q, character constants, and widths from arguments
printf 'abc%n|%s%n\n' n1 x n2; echo "$n1 $n2"
printf '  %s %s %s  \n%n' ab cd ef vv; echo "$vv"
f() { local loc; printf 'xy%n' loc; echo " $loc"; }; f
printf '[%q] [%Q] [%.3Q] [%#q]\n' 'a b' 'a b' 'abcdef' "it's"
printf '[%d] [%d] [%x] [%c]\n' "'A" '"b' "'é" "'"
LC_ALL=C.UTF-8; printf '[%d]\n' "'é"; LC_ALL=C
printf '[%*d] [%-*d] [%.*d] [%*.*s]\n' 5 1 5 2 4 3 6 2 abcdef
printf '[%*d]\n' -6 7
printf '%s %s\n' a b c
printf '[%5b] [%-5b] [%.2b]\n' 'a\tb' x 'abc'
printf '%b\n' 'a\cb' 'not printed'
echo status $?
