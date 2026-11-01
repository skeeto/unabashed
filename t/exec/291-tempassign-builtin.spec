== stdout 167
echo sees: old
eval sees: new
after: old
U=temp
U after: [unset]
X after colon: [unset]
Y after true: [unset]
declare -x Z="3"
Z after: [unset]
W=[unset] W2=5
x=b y=b

== stderr 0

== merged 167
echo sees: old
eval sees: new
after: old
U=temp
U after: [unset]
X after colon: [unset]
Y after true: [unset]
declare -x Z="3"
Z after: [unset]
W=[unset] W2=5
x=b y=b

== status 0
