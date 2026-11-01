declare -i x=1
echo "start x=$x"
x=1/0
echo "after div status=$? x=$x"
f() { local -i y; y=5%0; echo "in f status=$? y=$y"; }
f
echo "after f status=$?"
echo end
