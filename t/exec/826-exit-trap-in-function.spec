== stdout 142
trap:f:L:2
trap:g:G
st=3
in
trap::
out
trap::
trap:f
st=7
declare -- v
declare -- v="2+3"
declare -- v
declare -- v
declare -x v
declare -- v

== stderr 0

== merged 142
trap:f:L:2
trap:g:G
st=3
in
trap::
out
trap::
trap:f
st=7
declare -- v
declare -- v="2+3"
declare -- v
declare -- v
declare -x v
declare -- v

== status 0
