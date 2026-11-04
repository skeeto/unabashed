# vos
# This script itself has CRLF line endings
x=value
echo "[$x]"
if [ "$x" = value ]; then
    echo same
fi
for i in 1 2; do echo "$i"; done
cat <<EOF
heredoc $x
EOF
echo ${#x}
