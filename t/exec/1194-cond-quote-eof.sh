# A quote left open where [[ ]] expects && || ) or ]] after a complete
# term (a unary or binary test, a parenthesized one) adds bash's
# "unexpected EOF while looking for `]]'" at the line [[ began; one
# where an operator or operand is expected reports that instead.
while IFS= read -r c; do
    echo "-- $c"
    bash -c "$c" 2>&1; echo "status $?"
done <<'EOF2'
[[ -f a 'b
[[ ( a ) 'b
[[ ( a || b ) ]]'
[[ a == b "c
[[ a && -n b 'c
[[ a 'b
[[ 'a
[[ ( 'a
[[ -f 'a
[[ a ]] 'b
EOF2
