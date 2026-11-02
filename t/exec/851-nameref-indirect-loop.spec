== stdout 16
<>
<ok> <ok>
<>

== stderr 491
bash: line 2: warning: r1: maximum nameref depth (8) exceeded
bash: line 3: r1: invalid indirect expansion
bash: line 4: r2: invalid indirect expansion
bash: line 7: warning: a: circular name reference
bash: line 8: a: invalid indirect expansion
bash: line 9: a: invalid indirect expansion
bash: line 10: declare: warning: x: circular name reference
bash: line 10: warning: x: circular name reference
bash: line 10: x: invalid indirect expansion
bash: line 11: y: invalid indirect expansion

== merged 507
bash: line 2: warning: r1: maximum nameref depth (8) exceeded
<>
bash: line 3: r1: invalid indirect expansion
bash: line 4: r2: invalid indirect expansion
<ok> <ok>
bash: line 7: warning: a: circular name reference
<>
bash: line 8: a: invalid indirect expansion
bash: line 9: a: invalid indirect expansion
bash: line 10: declare: warning: x: circular name reference
bash: line 10: warning: x: circular name reference
bash: line 10: x: invalid indirect expansion
bash: line 11: y: invalid indirect expansion

== status 1
