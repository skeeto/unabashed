== stdout 158
st=0 [A\|\$|$(echo hi)|${x?boom}]
[A\|\$|$(echo hi)|${x?boom}]
[$((1/0)) ok]
[${y:?}] 0
[$nope]
sub 0
st 0
st 0
st 4
 1
[5] 0
[$((1/0))]
arith goes on
exit 1

== stderr 255
bash: line 6: x: boom
bash: line 7: x: boom
bash: line 8: 1/0: division by 0 (error token is "0")
bash: line 9: y: parameter null or not set
bash: line 10: nope: unbound variable
bash: line 17: 1/0: division by 0 (error token is "0")
bash: line 21: x: no

== merged 413
bash: line 6: x: boom
st=0 [A\|\$|$(echo hi)|${x?boom}]
bash: line 7: x: boom
[A\|\$|$(echo hi)|${x?boom}]
bash: line 8: 1/0: division by 0 (error token is "0")
[$((1/0)) ok]
bash: line 9: y: parameter null or not set
[${y:?}] 0
bash: line 10: nope: unbound variable
[$nope]
sub 0
st 0
st 0
st 4
 1
[5] 0
bash: line 17: 1/0: division by 0 (error token is "0")
[$((1/0))]
arith goes on
bash: line 21: x: no
exit 1

== status 1
