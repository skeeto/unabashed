# $( ( ) ) subshell vs arithmetic disambiguation
echo $( (echo subshell-in-cmdsub) )
echo $((1+2))
x=$( (exit 3) ); echo "st=$?"
echo $( ( echo a; echo b ) | { read l; echo "got $l"; } )
echo "$( (echo q) )"
y=$((echo hmm) 2>&1)
echo "y=$y"
