# export and readonly with -a or -A make an array only for a word with
# a value, never converting what exists; a conversion error found while
# a literal name=(...) expands abandons the command at top level, but
# is reported twice and passed over in a function
exec 2>&1
(
readonly -A a; declare -p a
readonly -a b; declare -p b
readonly -A c=([k]=v); declare -p c
readonly -a d=(1 2); declare -p d
e=1; readonly -A e; declare -p e
f=(1); readonly -A f; declare -p f
declare -A g; readonly -A g; declare -p g
declare -a h; readonly -A h; declare -p h
readonly -A; echo "st=$?"
declare -rA i; declare -p i
local_f() { local -rA j; declare -p j; }; local_f
export -A k 2>&1; declare -p k
)
(
readonly -a a=scalar; declare -p a
readonly -A b=scalar; declare -p b
readonly -A c=(1 2); declare -p c
e=1; readonly -a e=(x); declare -p e
f=(1); readonly -A f=([k]=v); declare -p f
g=(1); readonly -a g; declare -p g
export -a h=(1 2); declare -p h
export -A i=([k]=v); declare -p i
j=5; export -a j; declare -p j
declare -A k=([x]=y); export -a k; declare -p k
readonly -a; echo st=$?
export -a; echo st=$?
)
f=(1); declare -A f=([k]=v); echo "a $?"
f=(1); readonly -A f=([k]=v); echo "b $?"
echo "b2 $?"
g=(1); export -A g=([k]=v); echo "c $?"
echo "c2 $?"
h=(1); readonly -A h='([k]=v)'; echo "d $?"
fn() { local x=(1); local -A x=([k]=v); echo "e $?"; }; fn
declare -A k=([a]=b); readonly -a k=(1); echo "f $?"
echo "f2 $?"
r=(1); readonly -A r=([k]=v) s=1; echo "g $? $s"
echo "g2 $? ${s-unset}"
f=(1); declare -gA f=([k]=v); echo "a $?"
echo "a2 $?"
f=(1); declare -A f+=([k]=v); echo "b $?"
echo "b2 $?"
fn() { x=(1); declare -A x=([k]=v); echo "c $?"; }; fn; echo "c2 $?"
fn2() { declare -gA f=([k]=v); echo "d $?"; }; fn2; echo "d2 $?"
fn3() { local y=(1); declare -A y=([k]=v); echo "e $?"; }; fn3; echo "e2 $?"
fn4() { local -A z=([a]=b); local -a z=(1); echo "f $?"; }; fn4; echo "f2 $?"
for i in 1 2; do f=(1); declare -A f=([k]=v); echo "loop $i"; done; echo "after loop $?"
