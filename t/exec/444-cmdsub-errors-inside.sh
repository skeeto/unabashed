# Error messages from inside command substitutions and subshells
x=$(nosuchcmd_xyz)
echo "st=$? x=[$x]"
y=$(readonly r=1; r=2; echo after)
echo "y=[$y]"
z=$(echo ${unset_q:?is unset}; echo not)
echo "st=$? z=[$z]"
(echo ${unset_q2:?gone}; echo not2)
echo "st=$?"
w=$(cd /nowhere)
echo "st=$?"
