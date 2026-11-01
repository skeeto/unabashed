VAR=old
VAR=new echo "echo sees: $VAR"
VAR=new eval 'echo "eval sees: $VAR"'
echo "after: $VAR"
unset U
U=temp eval 'echo "U=$U"'
echo "U after: [${U-unset}]"
X=1 :
echo "X after colon: [${X-unset}]"
Y=2 true
echo "Y after true: [${Y-unset}]"
Z=3 declare -p Z 2>&1
echo "Z after: [${Z-unset}]"
W=4 export W2=5
echo "W=[${W-unset}] W2=$W2"
x=a; x=b y=$x; echo "x=$x y=$y"
