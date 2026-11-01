(( 1/0 ))
echo "a st=$?"
(( x = 4 % 0 ))
echo "b st=$?"
let 1/0
echo "c st=$?"
let 'y = 2/0' z=3
echo "d st=$? z=${z-unset}"
(( 1 + ))
echo "e st=$?"
let
echo "f st=$?"
for (( i = 0; i < 1/0; i++ )); do echo body; done
echo "g st=$?"
x=$(( 1 / 0 )) 2>/dev/null
echo "h st=$?"
echo end
