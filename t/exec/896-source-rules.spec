== stdout 124
args: x y
a b c
m n o p
f: 1 2
a b c
g: m n o p
a b c
in cwdf
status 1
in ./cwdf
two
one
one
in ./cwdf
nine
twelve
thirteen

== stderr 450
bash: line 13: .: cwdf: file not found
bash: line 1: notthere: No such file or directory
bash: line 1: .: notthere: file not found
bash: line 1: .: notthere: file not found
bash: line 1: .: notthere: file not found
bash: line 1: .: cwdf: file not found
bash: line 1: .: cwdf: file not found
bash: line 1: /nonexist/x: No such file or directory
bash: line 1: /nonexist/x: No such file or directory
bash: line 1: /nonexist/x: No such file or directory

== merged 574
args: x y
a b c
m n o p
f: 1 2
a b c
g: m n o p
a b c
in cwdf
bash: line 13: .: cwdf: file not found
status 1
in ./cwdf
bash: line 1: notthere: No such file or directory
two
bash: line 1: .: notthere: file not found
bash: line 1: .: notthere: file not found
one
bash: line 1: .: notthere: file not found
one
bash: line 1: .: cwdf: file not found
in ./cwdf
nine
bash: line 1: .: cwdf: file not found
bash: line 1: /nonexist/x: No such file or directory
bash: line 1: /nonexist/x: No such file or directory
twelve
bash: line 1: /nonexist/x: No such file or directory
thirteen

== status 0
