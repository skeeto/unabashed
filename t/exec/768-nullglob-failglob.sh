# nullglob drops words that match nothing; failglob makes them an
# error that abandons the command.
cd /tmp && mkdir ng && cd ng
touch a1 a2
shopt -s nullglob
set -- x* a* "b"* 'c'*d
echo "$#: $*"
arr=(none* a?)
echo "${#arr[@]} ${arr[*]}"
for f in q* r*; do echo "loop $f"; done
echo "empty for done"
echo [x] "[y]"
shopt -u nullglob
shopt -s failglob
echo a*
echo before; echo z*; echo "after $?"
echo z* || echo "or $?"
{ echo in group; echo z*; echo not reached; }
echo status $?
f() { echo z*; echo "in f $?"; }
f
echo "after f $?"
x=$(echo z*)
echo "comsub $? [$x]"
for g in z*; do echo nope; done
echo "for $?"
shopt -s nullglob
echo "both:" z* a*
