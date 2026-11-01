echo before
echo "value $(( 1 / 0 ))"
echo "after $?"
f() {
  echo "f1 $(( 1/0 ))"
  echo f2
}
f; echo same-line
echo "next-line $?"
eval 'echo "e1 $(( 1/0 ))"; echo e2
echo e3'; echo "after-eval-same-line $?"
( echo "s1 $(( 1/0 ))"; echo s2 ); echo "subshell status $?"
x=$(echo "c1 $(( 1/0 ))"; echo c2); echo "cmdsub [$x] $?"
for i in 1 2; do
  echo "loop $i $(( i/0 ))"
done
echo "after-loop $?"
{ echo "g1 $(( 1/0 ))"; echo g2; }
echo end
