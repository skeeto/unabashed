== stdout 63
ok 0
ok 1
after syntax error: 1
step 0
after step error: 1
end

== stderr 151
bash: line 3: ((: i < : arithmetic syntax error: operand expected (error token is "< ")
bash: line 5: ((: i += 1/0: division by 0 (error token is "0")

== merged 214
ok 0
ok 1
bash: line 3: ((: i < : arithmetic syntax error: operand expected (error token is "< ")
after syntax error: 1
step 0
bash: line 5: ((: i += 1/0: division by 0 (error token is "0")
after step error: 1
end

== status 0
