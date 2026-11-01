# return with too many arguments
f() { echo in; return 1 2; echo "after return"; }
f
echo "status $?"
echo end
