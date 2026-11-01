declare -A ass; ass=([k]=v x [j]=w); echo st=$?; declare -p ass
echo next
ass=([]=v); echo st=$?
echo next2
a=([-5]=x); echo st=$?
echo next3
declare -A d=([k]=v x [j]=w); echo st=$?; declare -p d
f() { local -A q=([k]=v x); echo st=$?; }; f
a=([@]=1); echo $?
declare -a z=([-1]=1); echo $?
h=(x); declare -A h; declare -p h
declare g=str; declare -A g; declare -p g; declare -a g; echo $?
b=(1); declare -A b; echo $?; declare -p b
