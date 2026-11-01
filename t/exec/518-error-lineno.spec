== stdout 85
a
b
deep
in:20
in2:21
a:21 b:22
c:23
one d:25
e:27 f:27
g:28
h:31
i:32
j:34 k:34
end

== stderr 375
bash: line 1: nosuch1: command not found
bash: line 5: nosuch2: command not found
bash: line 7: nosuch3: command not found
bash: line 10: nosuch4: command not found
bash: line 11: nosuch5: command not found
bash: line 12: nosuch6: command not found
bash: line 13: nosuch7: command not found
bash: line 16: ${bad x}: bad substitution
bash: line 18: nosuch8: command not found

== merged 460
bash: line 1: nosuch1: command not found
bash: line 5: nosuch2: command not found
a
b
bash: line 7: nosuch3: command not found
bash: line 10: nosuch4: command not found
bash: line 11: nosuch5: command not found
bash: line 12: nosuch6: command not found
bash: line 13: nosuch7: command not found
deep
bash: line 16: ${bad x}: bad substitution
bash: line 18: nosuch8: command not found
in:20
in2:21
a:21 b:22
c:23
one d:25
e:27 f:27
g:28
h:31
i:32
j:34 k:34
end

== status 0
