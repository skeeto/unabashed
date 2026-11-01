== stdout 420
declare -x var="60"
declare -rx tv='foo'
f1:3
global: unset
declare -x var="value"
declare -x var="one"
declare -a arr=([0]="zero" [1]="one" [2]="two" [3]="three")
declare -a arr=([0]="zero" [1]="one" [2]="two")
declare -x var="global"
declare -x var="global"
abc
declare -x VAR1
bash: line 15: local: PATH: not found
b
 
f2: local
after: global
f2: local
after: unset
ignoreeof           	on
ignoreeof           	on
10

== stderr 0

== merged 420
declare -x var="60"
declare -rx tv='foo'
f1:3
global: unset
declare -x var="value"
declare -x var="one"
declare -a arr=([0]="zero" [1]="one" [2]="two" [3]="three")
declare -a arr=([0]="zero" [1]="one" [2]="two")
declare -x var="global"
declare -x var="global"
abc
declare -x VAR1
bash: line 15: local: PATH: not found
b
 
f2: local
after: global
f2: local
after: unset
ignoreeof           	on
ignoreeof           	on
10

== status 0
