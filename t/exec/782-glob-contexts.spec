== stdout 129
*.txt one.txt
one.txt
cond literal
case literal
*.txt
hello
status 1
*.txt
*.txt
*.txt
for one.txt
for two.txt
*.txt
*.txt
*.txt

== stderr 41
bash: line 13: *.txt: ambiguous redirect

== merged 170
*.txt one.txt
one.txt
cond literal
case literal
*.txt
hello
bash: line 13: *.txt: ambiguous redirect
status 1
*.txt
*.txt
*.txt
for one.txt
for two.txt
*.txt
*.txt
*.txt

== status 0
