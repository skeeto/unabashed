# empty here-document bodies and odd delimiters
read x <<EOF2
EOF2
echo "status $? x=[$x]"
n=0; while read l; do n=$((n+1)); done <<EOF2

EOF2
echo "n=$n"
while IFS= read -r l; do echo "[$l]"; done <<'$x'
dollar delim
$x
while IFS= read -r l; do echo "[$l]"; done <<"a b"
quoted space delim
a b
while IFS= read -r l; do echo "[$l]"; done <<EOF2
EOF2x is not the end
 EOF2 with space is not either
EOF2
while IFS= read -r l; do echo "[$l]"; done <<''
empty delim

echo done
