echo start
echo "${x y}"; echo same-line-a
echo "next line runs"
f() { echo in-f; echo "${bad x}"; echo after-in-f; }
f; echo same-line-f
echo "after f st=$?"
for i in 1 2; do echo "loop $i"; : $((1/0)); echo "loop-after $i"; done
echo "after loop st=$?"
{ echo b1; : ${y z}; echo b2
  echo b3; }
echo "after brace"
eval 'echo e1; echo "${e x}"; echo e2
echo e3'; echo "same-line-eval st=$?"
echo 'echo s1; : ${s x}; echo s2
echo s3' > /tmp/src.sh
. /tmp/src.sh; echo "same-line-source st=$?"
readonly ro=1
ro=2; echo same-line-ro
echo "after ro st=$?"
echo end
