# Arithmetic evaluates a variable's value as an expression where it
# lies, and the expression may assign that very variable: the rest of
# the text is still read as it was, not from the freed string
x="(x=1)+x+5"; echo $((x))
x="x=1,x+5"; echo $((x))
x="x=12345678901234567,x+5"; echo $((x))
a=("a[0]=1,4"); echo $((a[0])) "${a[@]}"
declare -A h=([k]="h[k]=2,h[k]*3"); echo $((h[k])) "${h[k]}"
x="x=2,y=x*x"; ((x)); echo "$x $y"
x="x=3,x+1"; let 'r = x'; echo "$r $x"
x="x=4,x+1"; declare -i i; i=x; echo "$i $x"
x="(x=1)+x+5"; ( echo "sub $((x))" )
f() { local v="v=7,v*2"; echo $((v)); }; f
