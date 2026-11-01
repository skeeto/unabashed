== stdout 134
names 18
dollar-unset-plus 1
dollar-unset-alone 0
y->abc 12
z=12
w 13
minus-dollar -5
dollar-brace 20
positional
pos 56 3
unreachable

== stderr 87
bash: line 16: * 2 : arithmetic syntax error: operand expected (error token is "* 2 ")

== merged 221
names 18
dollar-unset-plus 1
dollar-unset-alone 0
y->abc 12
z=12
w 13
minus-dollar -5
dollar-brace 20
positional
pos 56 3
bash: line 16: * 2 : arithmetic syntax error: operand expected (error token is "* 2 ")
unreachable

== status 0
