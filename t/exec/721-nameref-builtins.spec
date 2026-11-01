== stdout 494
3 b c 0 1 2 b c 1 declare -a arr='a'
6
declare -a arr=([0]="a" [1]="5" [2]="c")
declare -a arr=([0]="x y" [1]="5" [2]="c")
declare -a arr=([0]="fmt" [1]="5" [2]="c")
? ? 
set
set2
tset
plus 
bash: line 10: declare: `1': invalid variable name for name reference
1
declare -a res=([0]="1" [1]="2" [2]="3")
inner: unset
outer: 
1 2
v mm
12
st=0
declare -n self2="self2"
fmt
12
[dflt] [assigned] assigned
gx=
declare -i x="9"
declare -ix x="9"
declare -n pr="x"
declare -ix x="9"
declare -- pr="x"

== stderr 39
bash: option requires an argument -- b

== merged 533
3 b c 0 1 2 b c 1 declare -a arr='a'
6
declare -a arr=([0]="a" [1]="5" [2]="c")
declare -a arr=([0]="x y" [1]="5" [2]="c")
declare -a arr=([0]="fmt" [1]="5" [2]="c")
bash: option requires an argument -- b
? ? 
set
set2
tset
plus 
bash: line 10: declare: `1': invalid variable name for name reference
1
declare -a res=([0]="1" [1]="2" [2]="3")
inner: unset
outer: 
1 2
v mm
12
st=0
declare -n self2="self2"
fmt
12
[dflt] [assigned] assigned
gx=
declare -i x="9"
declare -ix x="9"
declare -n pr="x"
declare -ix x="9"
declare -- pr="x"

== status 0
