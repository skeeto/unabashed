a() { echo a; }
declare -f a
b() { echo b; }
declare -f
echo done
exit 1
