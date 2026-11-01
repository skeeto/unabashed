== stdout 229
for cond true
for cond false
while cond (never ran) true
case no match true
case body false
subshell true
group false
nested if true
cond and arith
funcdef true
assignment true 5
assignment cmdsub 3
for in while 0
for in while 1

== stderr 0

== merged 229
for cond true
for cond false
while cond (never ran) true
case no match true
case body false
subshell true
group false
nested if true
cond and arith
funcdef true
assignment true 5
assignment cmdsub 3
for in while 0
for in while 1

== status 0
