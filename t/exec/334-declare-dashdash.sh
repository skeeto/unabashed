declare -- dd=1
declare -p -- dd
declare -i -- di=2+2
echo "di=$di"
local_dd() { local -- ld=-x; echo "ld=$ld"; }
local_dd
export -- ed=3
declare -p ed
unset -- dd
echo "dd=[${dd-unset}]"
declare -- -notopt=1
echo "status $?"
