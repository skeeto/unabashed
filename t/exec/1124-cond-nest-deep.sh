# [[ ]] nests parentheses as deep as bash's parse does, tens of
# thousands, to parse, run and print; an error reports once for each
# parenthesis it unwinds, as in bash
o= c=
for ((i = 0; i < 20000; i++)); do o+='( '; c+=' )'; done
eval "[[ $o x $c ]]" && echo nested
eval "[[ ! $o ! x && y $c || z ]]" && echo inverted
eval "[[ $o \"\" || ( y && $o x $c ) $c ]]" && echo operands
eval "f() { [[ $o x $c ]]; }"
declare -f f | wc -c
f && echo called
eval "[[ ${o:0:20000} x y ${c:0:20000} ]]" 2>&1 | sort | uniq -c | cut -c1-70
