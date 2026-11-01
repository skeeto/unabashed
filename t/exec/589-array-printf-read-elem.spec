== stdout 78
declare -a a=([0]="x" [2]="hi")
declare -- b="y"
declare -A h=(["k k"]="42" )

== stderr 0

== merged 78
declare -a a=([0]="x" [2]="hi")
declare -- b="y"
declare -A h=(["k k"]="42" )

== status 0
