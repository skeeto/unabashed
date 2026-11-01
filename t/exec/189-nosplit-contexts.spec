== stdout 119
1:<a   b>
1:<*>
1:<a   b*>
case-ok
case-lit
1:<a   b>
1:<g*>
1:<g*>
1:<g*>
1:<p  q r>
1:<p  q r>
1:<p  q:r>
1:<p  q:r>

== stderr 169
bash: line 19: $x: ambiguous redirect
bash: line 20: a   b: No such file or directory
bash: line 23: $f: ambiguous redirect
bash: line 24: g*: No such file or directory

== merged 288
1:<a   b>
1:<*>
1:<a   b*>
case-ok
case-lit
1:<a   b>
1:<g*>
bash: line 19: $x: ambiguous redirect
bash: line 20: a   b: No such file or directory
1:<g*>
bash: line 23: $f: ambiguous redirect
bash: line 24: g*: No such file or directory
1:<g*>
1:<p  q r>
1:<p  q r>
1:<p  q:r>
1:<p  q:r>

== status 0
