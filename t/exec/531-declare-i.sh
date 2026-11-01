declare -i x
x=2+3; echo "x=$x"
x='4 * 5'; echo "x=$x"
x+=10; echo "x+=10 -> $x"
x+='2*3'; echo "x+=2*3 -> $x"
y=7
x=y; echo "x=y -> $x"
x=y*2; echo "x=y*2 -> $x"
x=abc; echo "x=abc -> $x"
abc=9
x=abc; echo "x=abc (set) -> $x"
x=0x1f; echo "hex -> $x"
x=010; echo "oct -> $x"
x=; echo "empty -> '$x'"
declare -i z=3*3; echo "z=$z"
s=5; s+=3; echo "plain s=$s"
declare -i n=5; n+=3; echo "int n=$n"
declare -i m='n++'; echo "m=$m n=$n"
f() { local -i l=2**4; echo "local l=$l"; l+=1; echo "l=$l"; }
f
declare -i w=1 v=w+1; echo "w=$w v=$v"
declare -p x z
declare +i x; x=1+1; echo "unint x=$x"
