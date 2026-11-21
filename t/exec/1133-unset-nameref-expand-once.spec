== stdout 56
a 0 x z
b 0 x y
c 0 1 $(echo ran >&2) k
d 0 1 k
e 0 1 k

== stderr 129
ran
bash: line 19: $(echo ran >&2; echo 2): arithmetic syntax error: operand expected (error token is "$(echo ran >&2; echo 2)")

== merged 185
ran
a 0 x z
b 0 x y
c 0 1 $(echo ran >&2) k
d 0 1 k
e 0 1 k
bash: line 19: $(echo ran >&2; echo 2): arithmetic syntax error: operand expected (error token is "$(echo ran >&2; echo 2)")

== status 1
