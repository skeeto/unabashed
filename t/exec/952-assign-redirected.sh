# Assignments whose target is not the name written: through local and
# global namerefs, declare -g past a local, a nameref to an element or
# to a variable not yet set, readonly targets, and plain names around
# them that must still find their own entries
declare -n gref=target
f() { declare -g gref=val; }
f; echo "target=$target gref=$gref"
g() { local x=1; declare -n lr=x; lr=2; echo "x=$x"; }; g
arr=(a b c)
h() { declare -n r=arr[1]; r=Z; }; h; echo "arr=${arr[*]}"
declare -n up=newvar; up=created; echo "newvar=$newvar"
up=again; echo "newvar=$newvar up=$up"
readonly ro=1
( ro=2 ) 2>&1; echo "status $?"
declare -n rr=ro
( rr=3 ) 2>&1; echo "status $? ro=$ro"
f2() { local v=loc; declare -g v=glob; echo "in v=$v"; }; v=orig; f2; echo "out v=$v"
f3() { local -n lref=gv; lref=viaref; }; gv=; f3; echo "gv=$gv"
f4() { local gv=shadow; f3; echo "shadow gv=$gv"; }; f4; echo "global gv=$gv"
declare -n chain1=chain2 chain2=end; chain1=deep; echo "end=$end"
declare -i num; num=2+3; echo "num=$num"; num+=4; echo "num=$num"
s=1; s+=2; echo "s=$s"
for i in 1 2 3; do acc=$acc$i; done; echo "acc=$acc"
unset gv; f3; echo "gv=$gv"
declare -n empty
( empty=x ) 2>&1; echo "status $?"
