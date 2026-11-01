# read from here-strings and here-documents (text in memory): counts,
# delimiters, escapes, and what is left for the next read
read -n 0 x <<< ""; echo "a $? [$x]"
read -n 3 x <<< "ab"; echo "b $? [$x]"
read -n 3 x <<< "abc"; echo "c $? [$x]"
read -N 3 x <<< "ab"; echo "d $? [$x]"
read -N 3 x <<< "abcd"; echo "e $? [$x]"
read -d '' x <<< "abcd"; echo "f $? [$x]"
read -d c x <<< "abcd"; echo "g $? [$x]"
read x <<< 'a\b c'; echo "h $? [$x]"
read -r x y <<< '  a\b c  d  '; echo "i $? [$x] [$y]"
{ read -n 2 x; read y; } <<< "abcdef"; echo "j [$x] [$y]"
read -n 0 x < /dev/null; echo "k $? [$x]"
printf 'ab' | { read -n 0 x; echo "l $? [$x]"; }
read -n 2 -d b x <<< "abc"; echo "m $? [$x]"
IFS=: read -r a b c <<< "1:2:3:4"; echo "n [$a] [$b] [$c]"
read <<< "  lead trail  "; echo "o [$REPLY]"
{ read -d x a; read b; } <<'END'
one
twoxthree
four
END
echo "p [$a] [$b]"
{ read -n 4 a; cat; } <<< "12345678"; echo "q [$a]"
