== stdout 173
status 2
status 2
empty status 1
bracket arg status 0
-n status 0
! status 0
double status 2
extra status 2
test ] status 0
test x ] status 2
paren status 2
paren2 status 2

== stderr 249
bash: line 1: [: missing `]'
bash: line 2: [: missing `]'
bash: line 7: [: missing `]'
bash: line 8: [: x: unary operator expected
bash: line 10: test: x: unary operator expected
bash: line 11: [: missing `]'
bash: line 12: [: `)' expected, found ]

== merged 422
bash: line 1: [: missing `]'
status 2
bash: line 2: [: missing `]'
status 2
empty status 1
bracket arg status 0
-n status 0
! status 0
bash: line 7: [: missing `]'
double status 2
bash: line 8: [: x: unary operator expected
extra status 2
test ] status 0
bash: line 10: test: x: unary operator expected
test x ] status 2
bash: line 11: [: missing `]'
paren status 2
bash: line 12: [: `)' expected, found ]
paren2 status 2

== status 0
