== stdout 661
declare -a a=([0]="1" [1]="X" [2]="3" [3]="4")
declare -A h=([y]="2" )
declare -a a=([0]="1" [1]="2" [2]="3")
declare -A h=([x]="1" )
declare -a a=([0]="q")
declare -A h=()
declare -a a=([0]="1" [1]="2" [2]="3")
declare -A h=([x]="1" )
declare -a a=([0]="L" [5]="M")
declare -a a=([0]="1" [1]="2" [2]="3")
declare -a a=([0]="G" [1]="2" [2]="3")
declare -A h=([z]="3" [x]="1" )
bash: line 9: declare: a: not found
declare -a a=([0]="G" [1]="2" [2]="3")
declare -a b=([1]="1" [2]="4" [3]="9")
1 4 9 3
x y
declare -a x=([0]="ad" [1]="bc")
declare -a arr=([3]="v" [4]="w")
declare -- i="4"
declare -a z=([1]="1" [2]="5")
10 1
declare -A m=([b]="1" [a]="5" )
v v
sp

== stderr 0

== merged 661
declare -a a=([0]="1" [1]="X" [2]="3" [3]="4")
declare -A h=([y]="2" )
declare -a a=([0]="1" [1]="2" [2]="3")
declare -A h=([x]="1" )
declare -a a=([0]="q")
declare -A h=()
declare -a a=([0]="1" [1]="2" [2]="3")
declare -A h=([x]="1" )
declare -a a=([0]="L" [5]="M")
declare -a a=([0]="1" [1]="2" [2]="3")
declare -a a=([0]="G" [1]="2" [2]="3")
declare -A h=([z]="3" [x]="1" )
bash: line 9: declare: a: not found
declare -a a=([0]="G" [1]="2" [2]="3")
declare -a b=([1]="1" [2]="4" [3]="9")
1 4 9 3
x y
declare -a x=([0]="ad" [1]="bc")
declare -a arr=([3]="v" [4]="w")
declare -- i="4"
declare -a z=([1]="1" [2]="5")
10 1
declare -A m=([b]="1" [a]="5" )
v v
sp

== status 0
