== stdout 128
g: X=2
after g: X=2
after true: X=2
after cat: X=2
2
printenv: 0
top: X=unset
g: X=
k: A=outer Z=2 B=unset
top: Z=unset A=unset

== stderr 178
bash: line 8: X: readonly variable
bash: line 9: X: readonly variable
bash: line 10: X: readonly variable
bash: line 11: X: readonly variable
bash: line 15: Z: readonly variable

== merged 306
bash: line 8: X: readonly variable
g: X=2
after g: X=2
bash: line 9: X: readonly variable
after true: X=2
bash: line 10: X: readonly variable
after cat: X=2
bash: line 11: X: readonly variable
2
printenv: 0
top: X=unset
bash: line 15: Z: readonly variable
g: X=
k: A=outer Z=2 B=unset
top: Z=unset A=unset

== status 0
