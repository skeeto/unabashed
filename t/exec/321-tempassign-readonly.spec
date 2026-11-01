== stdout 52
f runs R=1
status 0
echo runs
status 0
status 0
R=1

== stderr 105
bash: line 3: R: readonly variable
bash: line 5: R: readonly variable
bash: line 7: R: readonly variable

== merged 157
bash: line 3: R: readonly variable
f runs R=1
status 0
bash: line 5: R: readonly variable
echo runs
status 0
bash: line 7: R: readonly variable
status 0
R=1

== status 0
