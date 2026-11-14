== stdout 31
sub 1
sub 1
sub 1
sub 1
exit 1

== stderr 161
bash: line 3: x: parameter not set
bash: line 4: x: parameter not set
bash: line 5: n: unbound variable
bash: line 6: nope: unbound variable
bash: line 9: x: no

== merged 192
bash: line 3: x: parameter not set
sub 1
bash: line 4: x: parameter not set
sub 1
bash: line 5: n: unbound variable
sub 1
bash: line 6: nope: unbound variable
sub 1
bash: line 9: x: no
exit 1

== status 1
