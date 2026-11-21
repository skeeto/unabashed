# bash assigns a declaration builtin's literal name=(...) while the
# words expand: xtrace shows each as name=('word' ...) with every word
# expanded and requoted, before any message about its elements, then
# the command with the name alone. Every word expands before any is
# assigned, so a subscript's expansion sees none of the arithmetic, and
# from declare an append sees none of the new elements. declare -n on
# such a name is reported in its place among the other names.
set -x
x=2
declare -a d=(a $x)
declare d2=(a "b c" $x) e=1
declare -A h=([k]=v [l]="m n")
declare -A h2=(k v l "m n")
f() { local l=(1 2); declare -g g=(3); }
f
export ex=(1)
readonly rx=(1 2)
declare -i n=(1+1 2)
d+=(more)
declare d+=(again)
typeset -a t=()
declare -- dd=(x)
declare 'q=(a b)'
declare -a w=([1]=a [3]=b)
b=(x)
declare b+=([2]+=y [5]=z w)
X=1 declare c=(1)
declare y=1 a=(p q) z=2
declare -A ha
declare ha+=([a]=1 [b]+=2)
declare -r r1=(1) r2=x
x=old; declare x=new aa=($x)
declare -A h3=([]=1 [k]=v)
declare -A h4=(k v '' w)
declare i=([]=1 [2]=b)
declare -a i2=([-5]=1 a)
declare -A h5=([a]=1)
declare h5+=([b]=2 [c]="${h5[b]}" []=3 [d]=4)
declare h5+=(e 5 f "${h5[e]}" '' 6 g 7)
declare -A h6=([a]=1 b)
declare -a i3=([@]=1 b)
n=5
declare -a i4=([n=3]=1 [$n]=2 [7]=$n)
fn() { declare -n nn=x xx=(1) yy=1bad; echo "fn $?"; }
fn
set +x
declare -p d d2 e h h2 g ex rx n t dd q w b c y a z ha r1 r2 aa
declare -p h3 h4 i i2 h5 h6 i3 i4 xx
n=5
i5=([n=3]=1 [$n]=2 [7]=$n)
declare -p i5
