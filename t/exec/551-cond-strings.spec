== stdout 402
T: [[ abc == abc ]]
T: [[ abc = abc ]]
F: [[ abc != abc ]] (1)
T: [[ abc != abd ]]
T: [[ abc ]]
F: [[ "" ]] (1)
T: [[ -z "" ]]
F: [[ -z x ]] (1)
F: [[ -n "" ]] (1)
T: [[ -n x ]]
F: [[ -n ]] (2)
F: [[ -z ]] (2)
F: [[ ! ]] (2)
F: [[ -f ]] (2)
T: [[ = ]]
T: [[ == ]]
T: [[ "" == "" ]]
T: [[ $unset == "" ]]
F: [[ -n $unset ]] (1)
T: [[ -z $unset ]]
F: [[ $unset ]] (1)
F: [[ $e ]] (1)
T: [[ "$e" == "" ]]

== stderr 520
bash: eval: line 1: unexpected argument `]]' to conditional unary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ -n ]]'
bash: eval: line 1: unexpected argument `]]' to conditional unary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ -z ]]'
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ ! ]]'
bash: eval: line 1: unexpected argument `]]' to conditional unary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ -f ]]'

== merged 922
T: [[ abc == abc ]]
T: [[ abc = abc ]]
F: [[ abc != abc ]] (1)
T: [[ abc != abd ]]
T: [[ abc ]]
F: [[ "" ]] (1)
T: [[ -z "" ]]
F: [[ -z x ]] (1)
F: [[ -n "" ]] (1)
T: [[ -n x ]]
bash: eval: line 1: unexpected argument `]]' to conditional unary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ -n ]]'
F: [[ -n ]] (2)
bash: eval: line 1: unexpected argument `]]' to conditional unary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ -z ]]'
F: [[ -z ]] (2)
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ ! ]]'
F: [[ ! ]] (2)
bash: eval: line 1: unexpected argument `]]' to conditional unary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ -f ]]'
F: [[ -f ]] (2)
T: [[ = ]]
T: [[ == ]]
T: [[ "" == "" ]]
T: [[ $unset == "" ]]
F: [[ -n $unset ]] (1)
T: [[ -z $unset ]]
F: [[ $unset ]] (1)
F: [[ $e ]] (1)
T: [[ "$e" == "" ]]

== status 0
