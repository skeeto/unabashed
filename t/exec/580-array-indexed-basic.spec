== stdout 485
declare -a a=([0]="1" [1]="2" [2]="3" [7]="x")
declare -A h=([b]="2" [a]="1" [zz]="q r" )
x 1

declare -a b=([0]="x" [2]="y")
declare -a c
declare -a c=()
declare -A e
declare -a l=([0]="1" [1]="2 3")
declare -A m
0 1 2 7 4 1
declare -ar r=([0]="1" [1]="2")
declare -ax ex=([0]="1")
declare -ai ai=([0]="2" [1]="9")
declare -ai ai=([0]="2" [1]="9" [2]="4")
declare -ai ai=([0]="7" [1]="9" [2]="4")
declare -a s=([0]="a" [5]="b" [6]="c")
declare -a s=([0]="a" [1]="x" [5]="by" [6]="z")

== stderr 78
bash: line 2: a: bad array subscript
bash: line 2: a[-9]: bad array subscript

== merged 563
declare -a a=([0]="1" [1]="2" [2]="3" [7]="x")
declare -A h=([b]="2" [a]="1" [zz]="q r" )
x 1
bash: line 2: a: bad array subscript

bash: line 2: a[-9]: bad array subscript
declare -a b=([0]="x" [2]="y")
declare -a c
declare -a c=()
declare -A e
declare -a l=([0]="1" [1]="2 3")
declare -A m
0 1 2 7 4 1
declare -ar r=([0]="1" [1]="2")
declare -ax ex=([0]="1")
declare -ai ai=([0]="2" [1]="9")
declare -ai ai=([0]="2" [1]="9" [2]="4")
declare -ai ai=([0]="7" [1]="9" [2]="4")
declare -a s=([0]="a" [5]="b" [6]="c")
declare -a s=([0]="a" [1]="x" [5]="by" [6]="z")

== status 0
