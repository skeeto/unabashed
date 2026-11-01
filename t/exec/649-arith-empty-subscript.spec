== stdout 62
1 0 
2 0 
3 0 
4 0 4
5 5 4
6 6 4
7 0 unset
8 0 4
9 1 4
10 1 4

== stderr 623
bash: line 4: ((: `a[]': not a valid identifier
bash: line 5: ((: `a[]': not a valid identifier
bash: line 6: let: `a[]': not a valid identifier
bash: line 8: `a[]': not a valid identifier
bash: line 9: `a[]': not a valid identifier
bash: line 10: ((: `b[]': not a valid identifier
bash: line 11: ((: `a[]': not a valid identifier
bash: line 12: a[]: bad array subscript
bash: line 12: a[]: bad array subscript
bash: line 12: ((: `a[]': not a valid identifier
bash: line 13: a[]: bad array subscript
bash: line 13: a[]: bad array subscript
bash: line 15: "": arithmetic syntax error: operand expected (error token is """")

== merged 685
bash: line 4: ((: `a[]': not a valid identifier
1 0 
bash: line 5: ((: `a[]': not a valid identifier
2 0 
bash: line 6: let: `a[]': not a valid identifier
3 0 
4 0 4
bash: line 8: `a[]': not a valid identifier
5 5 4
bash: line 9: `a[]': not a valid identifier
6 6 4
bash: line 10: ((: `b[]': not a valid identifier
7 0 unset
bash: line 11: ((: `a[]': not a valid identifier
8 0 4
bash: line 12: a[]: bad array subscript
bash: line 12: a[]: bad array subscript
bash: line 12: ((: `a[]': not a valid identifier
9 1 4
bash: line 13: a[]: bad array subscript
bash: line 13: a[]: bad array subscript
10 1 4
bash: line 15: "": arithmetic syntax error: operand expected (error token is """")

== status 1
