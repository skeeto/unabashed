== stdout 110
status 1

after
status 0

after
status 0
status 1
status 1
f
after
status 0
bcdef
after
status 0
status 1
end

== stderr 266
bash: line 3: -3: substring expression < 0
bash: line 6: x: 1/0: division by 0 (error token is "0")
bash: line 7: x: 2+: arithmetic syntax error: operand expected (error token is "+")
bash: line 10: x: a b: arithmetic syntax error in expression (error token is "b")

== merged 376
bash: line 3: -3: substring expression < 0
status 1

after
status 0

after
status 0
bash: line 6: x: 1/0: division by 0 (error token is "0")
status 1
bash: line 7: x: 2+: arithmetic syntax error: operand expected (error token is "+")
status 1
f
after
status 0
bcdef
after
status 0
bash: line 10: x: a b: arithmetic syntax error in expression (error token is "b")
status 1
end

== status 0
