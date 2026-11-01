declare -i gi=1
f() { local gi=3+4; echo "local gi=$gi"; declare -p gi; }
f
g() { gi=3+4; echo "global gi=$gi"; }
g
declare -u gu=x
h() { local gu=abc; echo "local gu=$gu"; local gu; echo "redecl gu=$gu"; }
h
declare -x gx=1
k() { local gx=2; declare -p gx; }
k
declare -p gi gu gx
outerro() { local -r cr=1; innerro; echo "outer cr=$cr"; }
innerro() { local cr=2; echo "inner cr=$cr status $?"; }
outerro
