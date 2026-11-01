# invalid for loop variable names
for 1x in a; do echo "body $1x"; done
echo "status $?"
f() { for a.b in q; do echo body; done; echo "f $?"; }
f
echo end
