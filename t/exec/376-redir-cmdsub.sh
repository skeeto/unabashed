# redirections inside command substitutions
x=$(echo to-stderr >&2)
echo "x=[$x]"
y=$( { echo out; echo err >&2; } 2>&1 )
echo "y=[$y]"
z=$(echo hidden >/tmp/f; echo shown)
echo "z=[$z]"
read r </tmp/f; echo "r=$r"
w=$(read l <<<"from-here"; echo "$l")
echo "w=$w"
v=$(while read l; do echo "<$l>"; done <<EOF2
one
two
EOF2
)
echo "v=$v"
u=$(echo nested 2>&1 >&-)
echo "status $? u=[$u]"
echo start; t=$(
echo two >&-
echo three >&-)
echo "t=[$t]"
