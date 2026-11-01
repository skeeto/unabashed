== stdout 149
start
next line runs
in-f
after f st=1
loop 1
after loop st=1
b1
after brace
e1
e3
same-line-eval st=0
s1
s3
same-line-source st=0
after ro st=1
end

== stderr 296
bash: line 2: ${x y}: bad substitution
bash: line 4: ${bad x}: bad substitution
bash: line 7: 1/0: division by 0 (error token is "0")
bash: line 9: ${y z}: bad substitution
bash: line 12: ${e x}: bad substitution
/tmp/src.sh: line 1: ${s x}: bad substitution
bash: line 18: ro: readonly variable

== merged 445
start
bash: line 2: ${x y}: bad substitution
next line runs
in-f
bash: line 4: ${bad x}: bad substitution
after f st=1
loop 1
bash: line 7: 1/0: division by 0 (error token is "0")
after loop st=1
b1
bash: line 9: ${y z}: bad substitution
after brace
e1
bash: line 12: ${e x}: bad substitution
e3
same-line-eval st=0
s1
/tmp/src.sh: line 1: ${s x}: bad substitution
s3
same-line-source st=0
bash: line 18: ro: readonly variable
after ro st=1
end

== status 0
