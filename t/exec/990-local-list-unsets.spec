== stdout 188
declare -a arr=([0]="x" [1]="y")
declare -a arr=([0]="n1" [1]="n2")
declare -a arr=([0]="n1" [1]="n2")
declare -A h=([k]="v" )
declare -a s=([0]="x")
declare -- s="old"
declare -- s="old"

== stderr 112
bash: line 6: declare: arr: not found
bash: line 12: declare: h: not found
bash: line 18: declare: z: not found

== merged 300
declare -a arr=([0]="x" [1]="y")
bash: line 6: declare: arr: not found
declare -a arr=([0]="n1" [1]="n2")
declare -a arr=([0]="n1" [1]="n2")
declare -A h=([k]="v" )
bash: line 12: declare: h: not found
declare -a s=([0]="x")
declare -- s="old"
declare -- s="old"
bash: line 18: declare: z: not found

== status 1
