return
echo "status $?"
return 5
echo "status $?"
f() { ( return 4 ); echo "subshell return: $?"; }
f
g() { x=$(return 6; echo nope); echo "cmdsub return: $? [$x]"; }
g
echo end
