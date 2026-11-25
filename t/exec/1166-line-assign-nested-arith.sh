# Assigning LINENO sets bash's line until the simple command ends; an
# arithmetic for takes the line of the last "for ((" read before its
# end, a nested one's; and a command substitution in the redirections
# of (( )), [[ ]] or an arithmetic for goes on from bash's line, though
# their messages cite the command's own
:
LINENO=7 <nope; echo $LINENO
x=$LINENO LINENO=20 y=$LINENO <nope; echo $x $y
LINENO=30 eval 'echo $LINENO; <nope'
for ((i=0; i<1; i++)); do
    for ((j=0; j<1; j++)); do :; done
    echo $LINENO
done <nope
x=$(for ((i=0; i<1; i++)); do for ((i=0; i<1; i++)); do echo $LINENO; done
echo $LINENO; done <nope
echo $LINENO)
echo "$x"
((1)) <<<"$(<nope
)"
((1 +
1)) <<<"$(<nope
)" 2>$((1.5))
echo "end $LINENO"
