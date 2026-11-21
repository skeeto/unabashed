# An integer variable's += evaluates the old value before the new one,
# which may assign the variable; and an assignment whose evaluation
# changes the variable stores into it as it is afterwards
declare -i n=5; n+="n=7"; echo $n
declare -i z=5; z+="z=7, q=99, 1"; echo "$z ${q-unset}"
x="y=3"; declare -i x; x+=1; echo "$x $y"
declare -ai a=(5); a[0]+="a[0]=7"; echo "${a[0]}"
y=0; declare -ai b=("y=4"); b[0]+="y*2"; echo "${b[0]} $y"
declare -i X=5; X+="X=7" declare -p X; echo $X
declare -i c=1; c="c[1]=5"; declare -p c
declare -i d=5; d+='y[${ d=10; }0]+7'; declare -p d
declare -ai e=(1 2); e[1]+='y[${ e=(4 5 6); }0]+7'; declare -p e
declare -ai g=(1 2); g[1]='y[${ unset g[1]; }0]+7'; declare -p g
declare -iA h=([k]=5); h+=([k]+='h[k]=7' [j]=1); declare -p h
declare -i s=5; ( s+="s=7"; echo "sub $s" )
f() { local -i v=5; v+="v=7"; echo "f $v"; }; f
