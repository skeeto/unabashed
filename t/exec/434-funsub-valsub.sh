# ${| cmd; } expands to the value of REPLY
x=${| REPLY=value; }
echo "[$x]"
REPLY=outer
y=${| REPLY=inner; echo printed-not-captured; }
echo "[$y] REPLY=$REPLY"
z=${| :; }
echo "[$z]"
b=1
w=${| b=2; REPLY=$b; }
echo "$w $b"
echo "dq: ${| REPLY='a  b'; }"
echo nodq: ${| REPLY='a  b'; }
v=${| REPLY=x; false; }
echo "status $?"
