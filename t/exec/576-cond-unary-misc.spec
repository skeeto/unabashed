== stdout 269
F: [[ -e $e ]]
F: [[ -f $e ]]
F: [[ -d $e ]]
F: [[ -r "" ]]
F: [[ -o errexit ]]
F: [[ -o nosuch ]]
T: [[ ! -o nosuch ]]
T: [[ -o nounset ]]
T: [[ -n -n ]]
F: [[ -z -z ]]
F: [[ -n == -n ]]
T: [[ -e /tmp && -d /tmp && ! -f /tmp ]]
T: [[ -s /tmp/nope || ! -s /tmp/nope ]]

== stderr 162
bash: eval: line 1: syntax error in conditional expression: unexpected token `-n'
bash: eval: line 1: syntax error near `-n'
bash: eval: line 1: `[[ -n == -n ]]'

== merged 431
F: [[ -e $e ]]
F: [[ -f $e ]]
F: [[ -d $e ]]
F: [[ -r "" ]]
F: [[ -o errexit ]]
F: [[ -o nosuch ]]
T: [[ ! -o nosuch ]]
T: [[ -o nounset ]]
T: [[ -n -n ]]
F: [[ -z -z ]]
bash: eval: line 1: syntax error in conditional expression: unexpected token `-n'
bash: eval: line 1: syntax error near `-n'
bash: eval: line 1: `[[ -n == -n ]]'
F: [[ -n == -n ]]
T: [[ -e /tmp && -d /tmp && ! -f /tmp ]]
T: [[ -s /tmp/nope || ! -s /tmp/nope ]]

== status 0
