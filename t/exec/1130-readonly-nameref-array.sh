# bash reports a readonly array assigned through a nameref, by a list or
# by an element, under the name as written; a scalar assignment names
# the target. Each failed assignment abandons the rest of its line.
readonly ro=1; declare -n rr=ro
rr=z; echo "a $?"
rr=(z); echo notreached
rr+=(z); echo notreached
rr[1]=z; echo notreached
rr[1]+=z; echo notreached
printf -v 'rr[1]' x; echo "b $?"
read 'rr[1]' <<<x; echo "c $?"
read -a rr <<<x; echo "d $?"
mapfile rr <<<x; echo "e $?"
: ${rr[1]=x}; echo notreached
declare rr=(z); echo notreached
readonly -a ra=(1); declare -n rb=ra
rb[0]=2; echo notreached
readonly -A rh=([a]=1); declare -n rhr=rh
rhr[b]=2; echo notreached
rhr+=([b]=2); echo notreached
declare -n r2=ro r3=r2
r3=(z); echo notreached
f() { local -n lr=ro; lr[2]=1; echo notreached; }; f
declare -A hh; declare -n hr=hh
mapfile hr <<<x; echo "f $?"
read -a hr <<<x; echo "g $?"
echo end
