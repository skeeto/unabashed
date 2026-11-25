== stdout 106
exit=1
s=0
exit=1
s=1
exit=127
s=127
exit=1
s=1
fine
s=0
s=1
s=1
---
[bye]
[bye]
[bye]
[bye]
[bye]
bye
[]

== stderr 495
bash: line 6: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
bash: line 7: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
bash: line 8: u: unset
bash: line 9: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
bash: line 11: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
bash: line 12: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")

== merged 601
bash: line 6: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
exit=1
s=0
bash: line 7: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
exit=1
s=1
bash: line 8: u: unset
exit=127
s=127
bash: line 9: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
exit=1
s=1
fine
s=0
bash: line 11: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
s=1
bash: line 12: 1.5: arithmetic syntax error: invalid arithmetic operator (error token is ".5")
s=1
---
[bye]
[bye]
[bye]
[bye]
[bye]
bye
[]

== status 0
