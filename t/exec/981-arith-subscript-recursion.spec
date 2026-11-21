== stdout 87
status 1
status 1
status 1
status 1
status 1
status 1
pipe 1 0
3
status 1
status 1
1 0

== stderr 769
bash: line 5: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
bash: line 6: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
bash: line 7: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
bash: line 8: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
bash: line 10: b[$((b[0]))]: expression recursion level exceeded (error token is "b[$((b[0]))]")
bash: line 12: c[0]: expression recursion level exceeded (error token is "c[0]")
bash: line 13: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
bash: line 18: w: expression recursion level exceeded (error token is "w")
bash: line 20: f[0]+1: expression recursion level exceeded (error token is "f[0]+1")

== merged 856
bash: line 5: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
status 1
bash: line 6: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
status 1
bash: line 7: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
status 1
bash: line 8: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
status 1
bash: line 10: b[$((b[0]))]: expression recursion level exceeded (error token is "b[$((b[0]))]")
status 1
bash: line 12: c[0]: expression recursion level exceeded (error token is "c[0]")
status 1
bash: line 13: a[a[0]]: expression recursion level exceeded (error token is "a[a[0]]")
pipe 1 0
3
bash: line 18: w: expression recursion level exceeded (error token is "w")
status 1
bash: line 20: f[0]+1: expression recursion level exceeded (error token is "f[0]+1")
status 1
1 0

== status 0
