== stdout 210
12
6 99
4 3
12
12 4
declare -x X="12"
7
declare -ai c=([0]="5" [1]="5")
declare -i d="16"
declare -ai e=([0]="4" [1]="13" [2]="6")
declare -ai g=([0]="1" [1]="11")
declare -Ai h=([k]="12" [j]="1" )
sub 12
f 12

== stderr 0

== merged 210
12
6 99
4 3
12
12 4
declare -x X="12"
7
declare -ai c=([0]="5" [1]="5")
declare -i d="16"
declare -ai e=([0]="4" [1]="13" [2]="6")
declare -ai g=([0]="1" [1]="11")
declare -Ai h=([k]="12" [j]="1" )
sub 12
f 12

== status 0
