== stdout 280
else status 1
two operands 1
missing rhs 1
assign non-var 1
unbalanced 127
unbalanced2 127
while status 0
bad hex 1
bad octal 1
bad digit 1
bad base 1
base one 1
neg exponent 1
abc 1
inc literal 0
postinc literal 1
for-test status 1
for-init status 1
body 0
for-step status 1
end

== stderr 1204
bash: line 1: ((: 1 + : arithmetic syntax error: operand expected (error token is "+ ")
bash: line 2: ((: 2 3 : arithmetic syntax error in expression (error token is "3 ")
bash: line 3: ((: x = : arithmetic syntax error: operand expected (error token is "= ")
bash: line 4: ((: 4 = 3 : attempted assignment to non-variable (error token is "= 3 ")
bash: line 5: 1: command not found
bash: line 6: 1: command not found
bash: line 7: ((: 1/0 : division by 0 (error token is "0 ")
bash: line 9: ((: 09: value too great for base (error token is "09")
bash: line 10: ((: 2#3: value too great for base (error token is "2#3")
bash: line 11: ((: 65#1: invalid arithmetic base (error token is "65#1")
bash: line 12: ((: 1#1: invalid arithmetic base (error token is "1#1")
bash: line 13: ((: 2 ** -1 : exponent less than 0 (error token is "1 ")
bash: line 14: ((: a b c : arithmetic syntax error in expression (error token is "b c ")
bash: line 16: ((: 5++ : arithmetic syntax error: operand expected (error token is "+ ")
bash: line 17: ((: i < 1/0: division by 0 (error token is "0")
bash: line 18: ((: i = 1/0: division by 0 (error token is "0")
bash: line 19: ((: i += 1/0 : division by 0 (error token is "0 ")

== merged 1484
bash: line 1: ((: 1 + : arithmetic syntax error: operand expected (error token is "+ ")
else status 1
bash: line 2: ((: 2 3 : arithmetic syntax error in expression (error token is "3 ")
two operands 1
bash: line 3: ((: x = : arithmetic syntax error: operand expected (error token is "= ")
missing rhs 1
bash: line 4: ((: 4 = 3 : attempted assignment to non-variable (error token is "= 3 ")
assign non-var 1
bash: line 5: 1: command not found
unbalanced 127
bash: line 6: 1: command not found
unbalanced2 127
bash: line 7: ((: 1/0 : division by 0 (error token is "0 ")
while status 0
bad hex 1
bash: line 9: ((: 09: value too great for base (error token is "09")
bad octal 1
bash: line 10: ((: 2#3: value too great for base (error token is "2#3")
bad digit 1
bash: line 11: ((: 65#1: invalid arithmetic base (error token is "65#1")
bad base 1
bash: line 12: ((: 1#1: invalid arithmetic base (error token is "1#1")
base one 1
bash: line 13: ((: 2 ** -1 : exponent less than 0 (error token is "1 ")
neg exponent 1
bash: line 14: ((: a b c : arithmetic syntax error in expression (error token is "b c ")
abc 1
inc literal 0
bash: line 16: ((: 5++ : arithmetic syntax error: operand expected (error token is "+ ")
postinc literal 1
bash: line 17: ((: i < 1/0: division by 0 (error token is "0")
for-test status 1
bash: line 18: ((: i = 1/0: division by 0 (error token is "0")
for-init status 1
body 0
bash: line 19: ((: i += 1/0 : division by 0 (error token is "0 ")
for-step status 1
end

== status 0
