== stdout 9
end
st 0

== stderr 311
s.sh: line 2: declare: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 5: `a b': not a valid identifier
s.sh: line 6: `c d': not a valid identifier
s.sh: line 7: export: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 8: `e f': not a valid identifier

== merged 320
s.sh: line 2: declare: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 5: `a b': not a valid identifier
s.sh: line 6: `c d': not a valid identifier
s.sh: line 7: export: 1+: arithmetic syntax error: operand expected (error token is "+")
s.sh: line 8: `e f': not a valid identifier
end
st 0

== status 0
