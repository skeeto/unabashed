== stdout 111
st=127
st=127
st=127
st=127
st=127
st=127
sub st=127
quiet st=127
pipe st=0
pipe2 st=127
var st=127
empty st=0

== stderr 429
bash: line 1: nosuch: command not found
bash: line 3: nosuch: command not found
bash: line 5: no such: command not found
bash: line 7: : command not found
bash: line 9: /no/such/path: No such file or directory
bash: line 11: ./nosuch: No such file or directory
bash: line 13: nosuch2: command not found
bash: line 17: nosuch4: command not found
bash: line 19: nosuch5: command not found
bash: line 21: nosuch6: command not found

== merged 540
bash: line 1: nosuch: command not found
st=127
bash: line 3: nosuch: command not found
st=127
bash: line 5: no such: command not found
st=127
bash: line 7: : command not found
st=127
bash: line 9: /no/such/path: No such file or directory
st=127
bash: line 11: ./nosuch: No such file or directory
st=127
bash: line 13: nosuch2: command not found
sub st=127
quiet st=127
bash: line 17: nosuch4: command not found
pipe st=0
bash: line 19: nosuch5: command not found
pipe2 st=127
bash: line 21: nosuch6: command not found
var st=127
empty st=0

== status 0
