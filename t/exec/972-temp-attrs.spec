== stdout 228
declare -x X="1+2"
declare -x X="1/0"
st 0
declare -x X="6"
7
in f [3*4]
eval [1+21]
declare -i X="5"
declare -x L="ABC"
declare -x L="xabc"
abc
declare -x A="x"
declare -x Y="1+2"
declare -ir R="1"
readonly 0
declare -ix X="3"

== stderr 91
bash: line 21: R: readonly variable
bash: line 22: 1/0: division by 0 (error token is "0")

== merged 319
declare -x X="1+2"
declare -x X="1/0"
st 0
declare -x X="6"
7
in f [3*4]
eval [1+21]
declare -i X="5"
declare -x L="ABC"
declare -x L="xabc"
abc
declare -x A="x"
declare -x Y="1+2"
bash: line 21: R: readonly variable
declare -ir R="1"
readonly 0
bash: line 22: 1/0: division by 0 (error token is "0")
declare -ix X="3"

== status 0
