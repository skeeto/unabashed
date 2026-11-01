== stdout 68
a st=1
b st=1
c st=1
d st=1 z=unset
e st=1
f st=1
g st=1
h st=1
end

== stderr 498
bash: line 1: ((: 1/0 : division by 0 (error token is "0 ")
bash: line 3: ((: x = 4 % 0 : division by 0 (error token is "0 ")
bash: line 5: let: 1/0: division by 0 (error token is "0")
bash: line 7: let: y = 2/0: division by 0 (error token is "0")
bash: line 9: ((: 1 + : arithmetic syntax error: operand expected (error token is "+ ")
bash: line 11: let: expression expected
bash: line 13: ((: i < 1/0: division by 0 (error token is "0")
bash: line 15: 1 / 0 : division by 0 (error token is "0 ")

== merged 566
bash: line 1: ((: 1/0 : division by 0 (error token is "0 ")
a st=1
bash: line 3: ((: x = 4 % 0 : division by 0 (error token is "0 ")
b st=1
bash: line 5: let: 1/0: division by 0 (error token is "0")
c st=1
bash: line 7: let: y = 2/0: division by 0 (error token is "0")
d st=1 z=unset
bash: line 9: ((: 1 + : arithmetic syntax error: operand expected (error token is "+ ")
e st=1
bash: line 11: let: expression expected
f st=1
bash: line 13: ((: i < 1/0: division by 0 (error token is "0")
g st=1
bash: line 15: 1 / 0 : division by 0 (error token is "0 ")
h st=1
end

== status 0
