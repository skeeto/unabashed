# NUL bytes in command substitution output are dropped with a warning
x=$(printf 'a\0b')
echo "[$x] ${#x}"
y=$(printf '\0\0c\0')
echo "[$y]"
echo "status $?"
