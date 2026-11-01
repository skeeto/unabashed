inner() { false; echo "inner continues"; }
outer() { inner; echo "outer continues"; false; echo "outer end"; }
set -e
if outer; then echo "if-true"; fi
outer && echo "and-true"
outer || echo "or"
! outer
echo "bang st=$?"
while outer; do break; done
x=$(outer) && echo "subst=[$x]"
echo "x=$x"
(outer) || echo "subshell or"
outer
echo notreached
