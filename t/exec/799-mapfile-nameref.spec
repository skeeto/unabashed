== stdout 237
declare -a target=([0]="a" [1]="b")
bash: line 3: warning: e: removing nameref attribute
declare -a e=()
bash: line 4: mapfile: `XXX[0]': not a valid identifier
1
bash: line 4: declare: XXX: not found
declare -a result=([0]="x" [1]="y")

== stderr 0

== merged 237
declare -a target=([0]="a" [1]="b")
bash: line 3: warning: e: removing nameref attribute
declare -a e=()
bash: line 4: mapfile: `XXX[0]': not a valid identifier
1
bash: line 4: declare: XXX: not found
declare -a result=([0]="x" [1]="y")

== status 0
