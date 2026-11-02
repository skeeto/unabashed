== stdout 193
1
3
declare -A a=(["80's"]="3" )
bash: line 10: let: a[x]y]=3: arithmetic syntax error: invalid arithmetic operator (error token is "]=3")
1
declare -A a=(["80's"]="3" )
declare -a i=([2]="4")

== stderr 0

== merged 193
1
3
declare -A a=(["80's"]="3" )
bash: line 10: let: a[x]y]=3: arithmetic syntax error: invalid arithmetic operator (error token is "]=3")
1
declare -A a=(["80's"]="3" )
declare -a i=([2]="4")

== status 0
