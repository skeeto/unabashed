== stdout 1089
plain: declare -A A=([x]="1" )
empty-first: declare -A A=([x]="1" ["x]"]="2" )
command: declare -A A=([x]="1" )
builtin: declare -A A=([x]="1" ["x]"]="2" )
expanded-name: declare -A A=([x]="1" ["x]"]="2" )
quoted: declare -A A=([x]="1" ["x]"]="2" )
command2: declare -A A=([x]="1" )
empty-command: declare -A A=([x]="1" ["x]"]="2" )
tempassign: declare -A A=([x]="1" )
declare -a arr=([1]="a" [2]="b")
read: declare -A A=(["x]"]="v" )
bash: line 26: read: `A[x]]': not a valid identifier
read-empty: declare -A A=()
read-command: declare -A A=(["x]"]="v" )
bash: line 28: read: `A[x]]': not a valid identifier
read-builtin: declare -A A=()
printf: declare -A A=(["x]"]="v" )
bash: line 30: printf: `A[x]]': not a valid identifier
printf-empty: declare -A A=()
bash: line 31: printf: `A[x]]': not a valid identifier
printf-expanded: declare -A A=()
b=1 2
bash: line 34: declare: `2': not a valid identifier
c=1
bash: line 35: declare: `2': not a valid identifier
d=1
bash: line 36: declare: `2': not a valid identifier
f=1
bash: line 37: export: `2': not a valid identifier
g=1
h=1 2
i=1 2

== stderr 0

== merged 1089
plain: declare -A A=([x]="1" )
empty-first: declare -A A=([x]="1" ["x]"]="2" )
command: declare -A A=([x]="1" )
builtin: declare -A A=([x]="1" ["x]"]="2" )
expanded-name: declare -A A=([x]="1" ["x]"]="2" )
quoted: declare -A A=([x]="1" ["x]"]="2" )
command2: declare -A A=([x]="1" )
empty-command: declare -A A=([x]="1" ["x]"]="2" )
tempassign: declare -A A=([x]="1" )
declare -a arr=([1]="a" [2]="b")
read: declare -A A=(["x]"]="v" )
bash: line 26: read: `A[x]]': not a valid identifier
read-empty: declare -A A=()
read-command: declare -A A=(["x]"]="v" )
bash: line 28: read: `A[x]]': not a valid identifier
read-builtin: declare -A A=()
printf: declare -A A=(["x]"]="v" )
bash: line 30: printf: `A[x]]': not a valid identifier
printf-empty: declare -A A=()
bash: line 31: printf: `A[x]]': not a valid identifier
printf-expanded: declare -A A=()
b=1 2
bash: line 34: declare: `2': not a valid identifier
c=1
bash: line 35: declare: `2': not a valid identifier
d=1
bash: line 36: declare: `2': not a valid identifier
f=1
bash: line 37: export: `2': not a valid identifier
g=1
h=1 2
i=1 2

== status 0
