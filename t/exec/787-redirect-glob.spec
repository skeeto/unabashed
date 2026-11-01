== stdout 102
content
[content]
status 1
status 1 []
split 1
posix status 1
posix comsub 1 []
posix split 1
literal

== stderr 280
bash: line 9: o*.txt: ambiguous redirect
bash: line 10: o*.txt: ambiguous redirect
bash: line 12: $f: ambiguous redirect
bash: line 14: o*.txt: No such file or directory
bash: line 15: o*.txt: No such file or directory
bash: line 16: only.txt other.txt: No such file or directory

== merged 382
content
[content]
bash: line 9: o*.txt: ambiguous redirect
status 1
bash: line 10: o*.txt: ambiguous redirect
status 1 []
bash: line 12: $f: ambiguous redirect
split 1
bash: line 14: o*.txt: No such file or directory
posix status 1
bash: line 15: o*.txt: No such file or directory
posix comsub 1 []
bash: line 16: only.txt other.txt: No such file or directory
posix split 1
literal

== status 0
