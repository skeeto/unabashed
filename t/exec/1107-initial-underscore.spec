== stdout 428
[foo]
declare -- _="[foo]"
declare -- _="_"
[bash]
declare -- _="[bash]"
declare -x _="/x/y"
declare -- _="_"
[]
declare -x _="/x/y"
declare -- _="declare -x _=\"/x/y\""
[bash] [s.sh]
[/q] [bash]
[bash] [bash]
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
declare -a _=([0]="./nosb" [1]="2")
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
1
1
0

== stderr 0

== merged 428
[foo]
declare -- _="[foo]"
declare -- _="_"
[bash]
declare -- _="[bash]"
declare -x _="/x/y"
declare -- _="_"
[]
declare -x _="/x/y"
declare -- _="declare -x _=\"/x/y\""
[bash] [s.sh]
[/q] [bash]
[bash] [bash]
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
declare -a _=([0]="./nosb" [1]="2")
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
declare -x _="./nosb"
1
1
0

== status 0
