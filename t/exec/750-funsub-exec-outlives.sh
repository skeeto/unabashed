# exec in ${ } keeps a descriptor on its output past the substitution:
# what is written there afterwards goes nowhere, and succeeds
x="${ exec 4>&1; echo in; echo four >&4; }"
echo "x=[$x]"
echo lost >&4; echo "st=$?"
y=$(echo sub >&4; echo "st=$?")
echo "y=[$y]"
f() { z="${ exec 5>&1; }"; }
f
echo five >&5; echo "st=$?"
x="${ exec 2>&1; echo err >&2; }"
echo "x=[$x]"
let "1 =="
echo "after: $?"
echo gone >&2; echo "st=$?"
exec 4>&- 5>&-
echo end
