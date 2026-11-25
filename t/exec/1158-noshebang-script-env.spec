== stdout 243
y=[] z=[2] t=[4] IFS=[ 	
] PS4=[+ ] OPTIND=1
./f2: line 2: declare: x: not found
./f2: line 2: declare: arr: not found
declare -fx g
shopt -u extglob
---
bash: line 1: declare: x: not found
z=[2]
no x
declare -x x
declare -x x
declare -x x=""

== stderr 0

== merged 243
y=[] z=[2] t=[4] IFS=[ 	
] PS4=[+ ] OPTIND=1
./f2: line 2: declare: x: not found
./f2: line 2: declare: arr: not found
declare -fx g
shopt -u extglob
---
bash: line 1: declare: x: not found
z=[2]
no x
declare -x x
declare -x x
declare -x x=""

== status 0
