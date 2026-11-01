# failglob makes a pattern with no match an expansion error that
# aborts the command.
cd /tmp
: > have
shopt -s failglob
echo *ave
echo *.none; echo "status $?"
echo before *.none after
echo "status $?"
x=*.none
echo "assign $x"
for f in *.none; do echo "in loop $f"; done
echo "after for $?"
echo "*.none"
f() { echo "f $*"; }
f *.none
echo "f status $?"
shopt -s nullglob
echo null *.none
shopt -u failglob
echo null2 *.none
echo end
