== stdout 5
st 1

== stderr 589
s.sh: line 1: ${y!!}: bad substitution
s.sh: line 3: declare: X: not found
s.sh: line 4: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 6: declare: Y: not found
s.sh: line 8: m=1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 9: declare: Z: not found
s.sh: line 9: declare: W: not found
s.sh: line 10: declare: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 11: declare: Q: not found
s.sh: line 12: ${y!!}: bad substitution
s.sh: line 13: declare: P: not found
s.sh: line 16: declare: R: not found

== merged 594
s.sh: line 1: ${y!!}: bad substitution
s.sh: line 3: declare: X: not found
s.sh: line 4: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 6: declare: Y: not found
s.sh: line 8: m=1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 9: declare: Z: not found
s.sh: line 9: declare: W: not found
s.sh: line 10: declare: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 11: declare: Q: not found
s.sh: line 12: ${y!!}: bad substitution
s.sh: line 13: declare: P: not found
s.sh: line 16: declare: R: not found
st 1

== status 0
