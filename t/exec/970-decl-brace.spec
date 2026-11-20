== stdout 235
st 1
declare -x X="a[]"
declare -x X="a"
declare -- b
declare -x X="~"
declare -x X="b"
declare -x X="X=*"
declare -x X="ac"
declare -x X="a b"
declare -x X="{a}"
declare -x X="/home/u/a"
declare -- Y="c"
declare -- d
declare -x X="*"

== stderr 104
bash: line 5: export: `a[]': not a valid identifier
bash: line 5: export: `a[]': not a valid identifier

== merged 339
bash: line 5: export: `a[]': not a valid identifier
bash: line 5: export: `a[]': not a valid identifier
st 1
declare -x X="a[]"
declare -x X="a"
declare -- b
declare -x X="~"
declare -x X="b"
declare -x X="X=*"
declare -x X="ac"
declare -x X="a b"
declare -x X="{a}"
declare -x X="/home/u/a"
declare -- Y="c"
declare -- d
declare -x X="*"

== status 0
