== stdout 488
bash: line 8: a[80's]: bad array subscript
bash: line 8: a[80's]: bad array subscript
bash: line 8: let: `a[80's]': not a valid identifier
s=0 
s=0 80's
bash: line 11: let: ++a[80's]: bad array subscript (error token is "a[80's]")
s=1 80's
bash: line 12: let: ++a[x"y]: bad array subscript (error token is "a[x"y]")
s=1 80's
bash: line 13: a[x"y]: bad array subscript
bash: line 13: a[x"y]: bad array subscript
bash: line 13: let: `a[x"y]': not a valid identifier
s=0 80's
s=1 80's x"y
1

== stderr 0

== merged 488
bash: line 8: a[80's]: bad array subscript
bash: line 8: a[80's]: bad array subscript
bash: line 8: let: `a[80's]': not a valid identifier
s=0 
s=0 80's
bash: line 11: let: ++a[80's]: bad array subscript (error token is "a[80's]")
s=1 80's
bash: line 12: let: ++a[x"y]: bad array subscript (error token is "a[x"y]")
s=1 80's
bash: line 13: a[x"y]: bad array subscript
bash: line 13: a[x"y]: bad array subscript
bash: line 13: let: `a[x"y]': not a valid identifier
s=0 80's
s=1 80's x"y
1

== status 0
