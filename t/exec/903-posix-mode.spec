== stdout 2645
< a:b c d e >
<x a:b c d e y>
< a><b><c d><e >
< a:b c d e >
< a:b><c d><e >
< a:b c d e >
<* a b>
<* a bx>
<>
<R>
<* a b a b>
<1 2:3 >
<1><2><3>
<>
<x>
< >
<a b><c>
<a><b><c>
<a><b><c>
bash
st 0
bash: line 1: !: parameter not set
st 127

st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
after
st 0
bash: line 1: 1/0 : division by 0 (error token is "0 ")
st 1
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 1
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
after
st 0
bash: line 1: ((: x+ : arithmetic syntax error: operand expected (error token is "+ ")
bash: line 1: let: x+: arithmetic syntax error: operand expected (error token is "+")
after
st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")

after
st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 127
bash: line 1: 1/0 : division by 0 (error token is "0 ")
st 127
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 127
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 127
bash: line 1: ((: x+ : arithmetic syntax error: operand expected (error token is "+ ")
bash: line 1: let: x+: arithmetic syntax error: operand expected (error token is "+")
after
st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")

after
st 0
<>
<>
<> <> <>
<>
<>
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 1
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 1
bash: line 1: x: x+: arithmetic syntax error: operand expected (error token is "+")
st 1
bash: line 1: nope: unbound variable
st 127
bash: line 1: x: boom
st 127
bash: line 1: n=1+: arithmetic syntax error: operand expected (error token is "+")
st 1
bash: line 1: r: readonly variable
st 1
bash: line 1: r: readonly variable
st 127
bash: line 1: x: bad
st 127
bash: line 1: x: bad
st 127
bash: line 1: x: bad
st 127
no out
bash: line 1: y: bad
or
st 0
bash: line 81: $i ${$'x1'}
: bad substitution
st 1
bash: line 81: $i ${$'x1'}
: bad substitution
st 1
bash: line 85: 1+: arithmetic syntax error: operand expected (error token is "+")
in 1
out 0
1 tOK
2 not t	b| a	b| 'a'| 3
f () 
{ 
    echo "3 ${x1%'t'}"
}
4 OK not tOK a	b
6 tOK $'a\tb'|
8 tOK tOK $'a\tb'| OK
9 	OK notOK
sourced
st 0
bash: line 1: .: sf: file not found
st 1
bash: line 1: source: sf: file not found
st 1
bash: line 1: .: sf: file not found
st 1
sourced
bash: line 1: .: sf: file not found
st 1
sourced x

== stderr 267
bash: line 94: ${'x1'}: bad substitution
bash: line 97: 7 ${$'x1'}: bad substitution
bash: line 103: ${$'x1'}: bad substitution
bash: line 106: 11 ${$'x1'}
: bad substitution
bash: line 109: ${'x1'%'t'}: bad substitution
bash: line 112: ${'x1'%'t'}: bad substitution

== merged 2912
< a:b c d e >
<x a:b c d e y>
< a><b><c d><e >
< a:b c d e >
< a:b><c d><e >
< a:b c d e >
<* a b>
<* a bx>
<>
<R>
<* a b a b>
<1 2:3 >
<1><2><3>
<>
<x>
< >
<a b><c>
<a><b><c>
<a><b><c>
bash
st 0
bash: line 1: !: parameter not set
st 127

st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
after
st 0
bash: line 1: 1/0 : division by 0 (error token is "0 ")
st 1
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 1
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
after
st 0
bash: line 1: ((: x+ : arithmetic syntax error: operand expected (error token is "+ ")
bash: line 1: let: x+: arithmetic syntax error: operand expected (error token is "+")
after
st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")

after
st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 127
bash: line 1: 1/0 : division by 0 (error token is "0 ")
st 127
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 127
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 127
bash: line 1: ((: x+ : arithmetic syntax error: operand expected (error token is "+ ")
bash: line 1: let: x+: arithmetic syntax error: operand expected (error token is "+")
after
st 0
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")

after
st 0
<>
<>
<> <> <>
<>
<>
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 1
bash: line 1: x+ : arithmetic syntax error: operand expected (error token is "+ ")
st 1
bash: line 1: x: x+: arithmetic syntax error: operand expected (error token is "+")
st 1
bash: line 1: nope: unbound variable
st 127
bash: line 1: x: boom
st 127
bash: line 1: n=1+: arithmetic syntax error: operand expected (error token is "+")
st 1
bash: line 1: r: readonly variable
st 1
bash: line 1: r: readonly variable
st 127
bash: line 1: x: bad
st 127
bash: line 1: x: bad
st 127
bash: line 1: x: bad
st 127
no out
bash: line 1: y: bad
or
st 0
bash: line 81: $i ${$'x1'}
: bad substitution
st 1
bash: line 81: $i ${$'x1'}
: bad substitution
st 1
bash: line 85: 1+: arithmetic syntax error: operand expected (error token is "+")
in 1
out 0
1 tOK
2 not t	b| a	b| 'a'| 3
f () 
{ 
    echo "3 ${x1%'t'}"
}
4 OK not tOK a	b
bash: line 94: ${'x1'}: bad substitution
6 tOK $'a\tb'|
bash: line 97: 7 ${$'x1'}: bad substitution
8 tOK tOK $'a\tb'| OK
9 	OK notOK
bash: line 103: ${$'x1'}: bad substitution
bash: line 106: 11 ${$'x1'}
: bad substitution
bash: line 109: ${'x1'%'t'}: bad substitution
bash: line 112: ${'x1'%'t'}: bad substitution
sourced
st 0
bash: line 1: .: sf: file not found
st 1
bash: line 1: source: sf: file not found
st 1
bash: line 1: .: sf: file not found
st 1
sourced
bash: line 1: .: sf: file not found
st 1
sourced x

== status 0
