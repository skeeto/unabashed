== stdout 464
declare -- x1="(1 2)"
declare -- x2="(1 2)"
declare -- x3="(1 2)"
declare -- x4="(3 4)"
bash: line 8: export: `a[0]': not a valid identifier
declare -x x5="(1 2)"
bash: line 9: readonly: `a[0]': not a valid identifier
declare -r x6="(1 2)"
declare -- x7="(3 4)"
declare -- x8="(5 6)"
declare -- x9="(7 8)"
declare -- x10="(9 0)"
declare -ar ro=([0]="1" [1]="2")
bash: line 19: declare: ro: readonly variable
declare -ar ro=([0]="1" [1]="2")
declare -a b=([0]="1")

== stderr 0

== merged 464
declare -- x1="(1 2)"
declare -- x2="(1 2)"
declare -- x3="(1 2)"
declare -- x4="(3 4)"
bash: line 8: export: `a[0]': not a valid identifier
declare -x x5="(1 2)"
bash: line 9: readonly: `a[0]': not a valid identifier
declare -r x6="(1 2)"
declare -- x7="(3 4)"
declare -- x8="(5 6)"
declare -- x9="(7 8)"
declare -- x10="(9 0)"
declare -ar ro=([0]="1" [1]="2")
bash: line 19: declare: ro: readonly variable
declare -ar ro=([0]="1" [1]="2")
declare -a b=([0]="1")

== status 0
