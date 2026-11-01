== stdout 46
st=1
st=1
st=1
st=1
st=1
st=1
st=1
notreached

== stderr 472
bash: line 1: 1/0: division by 0 (error token is "0")
bash: line 2: 5%0: division by 0 (error token is "0")
bash: line 3: 10/x: division by 0 (error token is "x")
bash: line 4: 2/0: division by 0 (error token is "0")
bash: line 5: 1+: arithmetic syntax error: operand expected (error token is "+")
bash: line 6: 08: value too great for base (error token is "08")
bash: line 7: x/=0: division by 0 (error token is "0")
bash: line 8: 3/0: division by 0 (error token is "0")

== merged 518
bash: line 1: 1/0: division by 0 (error token is "0")
st=1
bash: line 2: 5%0: division by 0 (error token is "0")
st=1
bash: line 3: 10/x: division by 0 (error token is "x")
st=1
bash: line 4: 2/0: division by 0 (error token is "0")
st=1
bash: line 5: 1+: arithmetic syntax error: operand expected (error token is "+")
st=1
bash: line 6: 08: value too great for base (error token is "08")
st=1
bash: line 7: x/=0: division by 0 (error token is "0")
st=1
bash: line 8: 3/0: division by 0 (error token is "0")
notreached

== status 0
