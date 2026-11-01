# Basic $( ) and backquote command substitution, trailing newlines
x=$(echo hello)
echo "[$x]"
y=`echo world`
echo "[$y]"
z=$(printf 'a\n\n\n')
echo "[$z]"
w=$(printf '\n\na\n\nb\n\n')
echo "[$w]"
e=$(:)
echo "[$e]" "${#e}"
n=$(printf 'x\r\n')
echo "${#n}"
echo "[$(printf '  sp  ')]"
echo [$(printf 'a  b\tc\n')]
