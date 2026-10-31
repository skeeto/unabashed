echo one
f() { echo f; }
declare -f
echo two
done
echo never
