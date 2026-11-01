== stdout 206
one a[1]
declare -a a=([0]="zero" [1]="uno" [2]="two")
unoX
declare -a a=([0]="zero" [2]="two")
<zero><two>
<zero><two>
zero two three seven 4 0 2 3 7
two three seven
v w 2
V
declare -a q=([0]="1" [1]="2")

== stderr 142
bash: line 20: `': not a valid identifier
bash: line 21: warning: q: removing nameref attribute
bash: line 22: `a[0]': not a valid identifier

== merged 348
one a[1]
declare -a a=([0]="zero" [1]="uno" [2]="two")
unoX
declare -a a=([0]="zero" [2]="two")
<zero><two>
<zero><two>
zero two three seven 4 0 2 3 7
two three seven
v w 2
V
bash: line 20: `': not a valid identifier
bash: line 21: warning: q: removing nameref attribute
declare -a q=([0]="1" [1]="2")
bash: line 22: `a[0]': not a valid identifier

== status 1
