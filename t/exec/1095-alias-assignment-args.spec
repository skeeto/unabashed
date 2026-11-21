== stdout 1202
alias q1='a  b a*'
alias q2='a*'
alias q3='b'
alias q4='/h:a:/h/x'
alias q5='a~'
alias q6+='a  b a*'
alias q7[1]='a'
alias q8='a  b a*'
bash: line 13: alias: b: not found
bash: line 13: alias: ab: not found
bash: line 13: alias: ac: not found
alias q9='a'
bash: line 15: alias: b: not found
bash: line 15: alias: ab: not found
bash: line 15: alias: ac: not found
bash: line 15: alias: b: not found
bash: line 15: alias: ab: not found
bash: line 15: alias: ac: not found
bash: line 15: alias: b: not found
bash: line 15: alias: ab: not found
bash: line 15: alias: ac: not found
bash: line 16: alias: b: not found
bash: line 16: alias: ab: not found
bash: line 16: alias: ac: not found
bash: line 16: alias: b: not found
bash: line 16: alias: ab: not found
bash: line 16: alias: ac: not found
alias r1='a'
alias r2='a'
alias r3='a'
alias r4='a'
alias r5='a'
alias r6='a'
<f1=a  b a*>
status 1
declare -a c1=([0]="a" [1]="2")
status 1
declare -a c2=([0]="o" [1]="c")
alias c3='a  b a*'
declare -a c4=([3]="a" [4]="b")
bash: line 25: declare: c4: not found
status 1
p1='a  b a*'
p2='a  b a*'
bash: line 33: alias: b: not found
bash: line 33: alias: ab: not found
bash: line 33: alias: ac: not found
p3='a'

== stderr 454
bash: line 10: alias: b: not found
bash: line 10: alias: ab: not found
bash: line 10: alias: ac: not found
bash: line 17: alias: b: not found
bash: line 17: alias: ab: not found
bash: line 17: alias: ac: not found
bash: line 22: alias: c1: not found
bash: line 23: alias: c2: not found
bash: line 24: alias: c4: not found
bash: line 27: h: 'x': must use subscript when assigning associative array
bash: line 28: c5[1]: cannot assign list to array member

== merged 1656
bash: line 10: alias: b: not found
bash: line 10: alias: ab: not found
bash: line 10: alias: ac: not found
alias q1='a  b a*'
alias q2='a*'
alias q3='b'
alias q4='/h:a:/h/x'
alias q5='a~'
alias q6+='a  b a*'
alias q7[1]='a'
alias q8='a  b a*'
bash: line 13: alias: b: not found
bash: line 13: alias: ab: not found
bash: line 13: alias: ac: not found
alias q9='a'
bash: line 15: alias: b: not found
bash: line 15: alias: ab: not found
bash: line 15: alias: ac: not found
bash: line 15: alias: b: not found
bash: line 15: alias: ab: not found
bash: line 15: alias: ac: not found
bash: line 15: alias: b: not found
bash: line 15: alias: ab: not found
bash: line 15: alias: ac: not found
bash: line 16: alias: b: not found
bash: line 16: alias: ab: not found
bash: line 16: alias: ac: not found
bash: line 16: alias: b: not found
bash: line 16: alias: ab: not found
bash: line 16: alias: ac: not found
bash: line 17: alias: b: not found
bash: line 17: alias: ab: not found
bash: line 17: alias: ac: not found
alias r1='a'
alias r2='a'
alias r3='a'
alias r4='a'
alias r5='a'
alias r6='a'
<f1=a  b a*>
bash: line 22: alias: c1: not found
status 1
declare -a c1=([0]="a" [1]="2")
bash: line 23: alias: c2: not found
status 1
declare -a c2=([0]="o" [1]="c")
alias c3='a  b a*'
bash: line 24: alias: c4: not found
declare -a c4=([3]="a" [4]="b")
bash: line 25: declare: c4: not found
bash: line 27: h: 'x': must use subscript when assigning associative array
bash: line 28: c5[1]: cannot assign list to array member
status 1
p1='a  b a*'
p2='a  b a*'
bash: line 33: alias: b: not found
bash: line 33: alias: ab: not found
bash: line 33: alias: ac: not found
p3='a'

== status 0
