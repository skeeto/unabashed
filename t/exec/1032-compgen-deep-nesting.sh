# compgen -W splits its words keeping quotes and expansions whole, and
# then expands each: nesting deep enough to exhaust a recursive scan is
# one unterminated word, a bad substitution as in bash
w='${'
for i in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17; do w=$w$w; done
echo "${#w}"
( compgen -W "$w" x; echo not reached ) 2>/dev/null; echo "status $?"
( compgen -W "a $w" a; echo not reached ) 2>/dev/null; echo "status $?"
# shallow nesting splits as before
w='$(echo "$(echo "$(echo a b)")") ${x:-"}"} c'
compgen -W "$w" ''
compgen -W "$w" c
echo end
