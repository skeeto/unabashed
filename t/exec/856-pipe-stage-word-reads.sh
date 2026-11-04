# An external pipeline stage whose words read the stage's input while
# they expand: funsubs, prompt strings and subscripts that run a comsub
echo a | cat ${ cat; } | cat; echo "1 $?"
echo b | cat "${ cat; }" - | cat; echo "2 $?"
echo c | cat ${	read v; echo "$v"; } - | cat; echo "3 $?"
echo d | cat ${| REPLY=$(cat); } - | cat; echo "4 $?"
f() { sed s/^/f:/ >&2; }
echo e | cat ${ f; } - | cat; echo "5 $?"
p='$(cat >&2)'
echo f | cat "${p@P}" - | cat; echo "6 $?"
i='a[$(cat >&2)]'
echo g | cat ${a[i]} - | cat; echo "7 $?"
echo h | cat $((i)) - 2>/dev/null | cat; echo "8 $?"
r='a[$(cat >&2)]'
echo i | cat ${!r} - | cat; echo "9 $?"
true | od ${ sed p; } | true; echo "10 $?"
seq 3000 | wc -l ${ wc -l >&2; } - | cat; echo "11 $?"
echo j | cat ${ cat >&2; } - | cat - ${ echo k | cat >/dev/null; } | cat; echo "12 $?"
echo l | cat ${ cat; } - | cat ${ cat; } - | cat; echo "13 $?"
echo m | cat ${ cat; } | cat ${ cat; } - ; echo "14 $?"
echo n | cat $(cat >&2) - | cat; echo "15 $?"
echo o | cat <(cat) | cat; echo "16 $?"
cat ${ cat; } - <<<q | cat; echo "17 $?"
{ echo r | cat ${ cat; } - | cat; } 2>&1 | cat ${ cat; } - | cat; echo "18 $?"
