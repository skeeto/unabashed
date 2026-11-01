== stdout 31
start
else 1
after 0
div 1
end

== stderr 143
bash: line 2: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
bash: line 4: [[: 1/0: division by 0 (error token is "0")

== merged 174
start
bash: line 2: [[: 1+: arithmetic syntax error: operand expected (error token is "+")
else 1
after 0
bash: line 4: [[: 1/0: division by 0 (error token is "0")
div 1
end

== status 0
