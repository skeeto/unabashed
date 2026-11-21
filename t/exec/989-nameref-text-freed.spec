== stdout 343
result: [one]
declare -a a=([0]="zero" [1]="new" [2]="two")
declare -a a=([0]="zero" [1]="onenew" [2]="two")
declare -a a=([0]="zero" [2]="two")
declare -a a=([0]="zero" [1]="pv" [2]="two")
declare -a a=([0]="zero" [1]="rd" [2]="two")
declare -A h=([k]="new" )
q
abc
declare -- x="zzzzzzzz"
declare -- zzzzzzzz="abc"
x=1 q=unset
y=unset
unset

== stderr 129
bash: line 23: 1: invalid variable name
bash: line 25: !x: invalid indirect expansion
bash: line 27: a[5]: invalid variable name

== merged 472
result: [one]
declare -a a=([0]="zero" [1]="new" [2]="two")
declare -a a=([0]="zero" [1]="onenew" [2]="two")
declare -a a=([0]="zero" [2]="two")
declare -a a=([0]="zero" [1]="pv" [2]="two")
declare -a a=([0]="zero" [1]="rd" [2]="two")
declare -A h=([k]="new" )
q
abc
declare -- x="zzzzzzzz"
declare -- zzzzzzzz="abc"
bash: line 23: 1: invalid variable name
x=1 q=unset
bash: line 25: !x: invalid indirect expansion
y=unset
bash: line 27: a[5]: invalid variable name
unset

== status 0
