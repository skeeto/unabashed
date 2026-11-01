set -e
if false; then echo no; else echo else; fi
while false; do echo no; done
until true; do echo no; done
! true
echo "bang st=$?"
! false
echo "bang2 st=$?"
if ! false; then echo yes; fi
false && echo no
echo "and st=$?"
false || false || true
echo "or st=$?"
true && false || echo rescued
x=$(false) || echo "assign rescued"
elif_test() { if false; then :; elif false; then :; else echo elif-else; fi; }
elif_test
echo end
true && false
echo notreached
