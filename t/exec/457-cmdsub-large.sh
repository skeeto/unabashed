# Large command substitution output
gen() { i=0; while [ $i -lt 3000 ]; do echo "line $i"; i=$((i+1)); done; }
x=$(gen)
echo "${#x}"
echo "${x%%$'\n'*}"
echo "${x##*$'\n'}"
y=$(printf '%5000s' '')
echo "${#y}"
