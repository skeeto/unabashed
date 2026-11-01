== stdout 377
declare -A A=(["\$key"]="1" ["x],b[\$(echo ran >&2)"]="2" )
3
v 0
v 0
v 1
eq 0
declare -A A=(["\$key"]="1" )
declare -A A=(["\$(echo foo)"]="1" ["\$key"]="1" )
declare -A A=(["\$key"]="1" )
test 1
shopt -s array_expand_once
test 0
declare -a a=([2]="5" [3]="6")
declare -a a=([0]="3" [2]="5" [3]="6")
declare -a a=([1]="x" [2]="y" [3]="z")
bash: line 34: declare: a: not found

== stderr 111
bash: line 22: unset: `A[$(echo': not a valid identifier
bash: line 22: unset: `foo)]': not a valid identifier

== merged 488
declare -A A=(["\$key"]="1" ["x],b[\$(echo ran >&2)"]="2" )
3
v 0
v 0
v 1
eq 0
declare -A A=(["\$key"]="1" )
declare -A A=(["\$(echo foo)"]="1" ["\$key"]="1" )
declare -A A=(["\$key"]="1" )
bash: line 22: unset: `A[$(echo': not a valid identifier
bash: line 22: unset: `foo)]': not a valid identifier
test 1
shopt -s array_expand_once
test 0
declare -a a=([2]="5" [3]="6")
declare -a a=([0]="3" [2]="5" [3]="6")
declare -a a=([1]="x" [2]="y" [3]="z")
bash: line 34: declare: a: not found

== status 1
