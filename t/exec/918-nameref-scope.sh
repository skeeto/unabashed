# A scalar assignment through a nameref sets the binding seen from the
# nameref's own scope, even when a newer function shadows it, while
# reading, elements and compound assignments use the visible binding
# (bash's bind_variable against its array paths)
exec 2>&1
x=global
f() { local -n r=x; g; echo "f: x=$x r=$r"; }
g() { local x=g-local; r=via-ref; echo "g: x=$x r=$r"; }
f; echo "top $x"
f2() { local x=f2; local -n r=x; g2; echo "f2: x=$x"; }
g2() { local x=g2; r=via-ref2; echo "g2: x=$x r=$r"; }
f2; echo "top $x"
f3() { local -n r=x; g3; }
g3() { local x=g3; unset r; echo "g3: x=${x-unset}"; }
f3; echo "top ${x-unset}"
x=back
f4() { local -n r=x; g4; echo "f4 x=$x"; }
g4() { local x=g4; local r=loc; echo "g4: x=$x r=$r"; }
f4
readonly zz=1; f5() { typeset -n q=zz; typeset q=2; echo "s=$?"; q=3; echo "s=$?"; }; f5
f() { local x=f; local -a arr=(f0 f1); local -n r=x a=arr; g; echo "f: x=$x arr=${arr[*]}"; }
g() { local x=g; local -a arr=(g0 g1); r+=+; echo "1 $x"; read r <<<rd; echo "2 $x"; printf -v r %s pv; echo "3 $x"
  (( r=5 )); echo "4 $x"; for r in L1 L2; do :; done; echo "5 $x"; a[1]=A; echo "6 ${arr[*]}"; a=(n0 n1); echo "7 ${arr[*]}"
  a+=(more); echo "8 ${arr[*]}"; r=end; declare -p x arr; }
f
h() { local -i n=1; local -n r=n; k; echo "h n=$n"; }
k() { local n=k; r=2+3; echo "k n=$n"; }
h
