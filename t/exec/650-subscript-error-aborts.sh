# an error evaluating an array subscript abandons the whole -c text
# (as a function, eval or source), unlike other expansion errors
bash -c 'echo $((1/0)); echo a1
echo a2'
echo "st $?"
bash -c 'shopt -s assoc_expand_once; declare -a a; let "a[\"\"]=1"; echo b1
echo b2'
echo "st $?"
bash -c 'declare -a a; let "a[1/0]=1"; echo c1
echo c2'
echo "st $?"
bash -c 'declare -a a; (( a[1/0]=1 )); echo d1
echo d2'
echo "st $?"
bash -c 'declare -a a; x=$(( a[1/0] )); echo e1
echo e2'
echo "st $?"
bash -c 'declare -a a; echo ${a[1/0]}; echo f1
echo f2'
echo "st $?"
bash -c 'declare -a a; a[1/0]=x; echo g1
echo g2'
echo "st $?"
bash -c 'declare -a a; f() { (( a[1/0] )); echo h0; }; f; echo h1
echo h2'
echo "st $?"
bash -c 'declare -a a; eval "(( a[1/0] )); echo i0"; echo i1
echo i2'
echo "st $?"
bash -c 'declare -a a; ( (( a[1/0] )); echo j0 ); echo j1
echo j2'
echo "st $?"
bash -c 'declare -a a; echo $(( a[1/0] )); echo k1
echo k2'
echo "st $?"
