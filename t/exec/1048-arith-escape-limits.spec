== stdout 45
n=360
status 1
n=360
status 1
n=511
status 1

== stderr 361
bash: line 11: u: parameter not set
bash: line 11: n++, b[0]: expression recursion level exceeded (error token is "n++, b[0]")
bash: line 13: n++, b[0]: expression recursion level exceeded (error token is "n++, b[0]")
bash: line 15: u: parameter not set
bash: line 16: b[$((n++, b[0]))]: expression recursion level exceeded (error token is "b[$((n++, b[0]))]")

== merged 406
bash: line 11: u: parameter not set
bash: line 11: n++, b[0]: expression recursion level exceeded (error token is "n++, b[0]")
n=360
status 1
bash: line 13: n++, b[0]: expression recursion level exceeded (error token is "n++, b[0]")
n=360
status 1
bash: line 15: u: parameter not set
bash: line 16: b[$((n++, b[0]))]: expression recursion level exceeded (error token is "b[$((n++, b[0]))]")
n=511
status 1

== status 0
