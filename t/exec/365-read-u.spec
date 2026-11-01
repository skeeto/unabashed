== stdout 92
a=l1 b=l2
c=l3
status 1 d=[]
status 1
status 1
status 1
status 1
status 1
status 1
status 1

== stderr 453
bash: line 8: read: 5: invalid file descriptor: Bad file descriptor
bash: line 9: read: abc: invalid file descriptor specification
bash: line 10: read: -1: invalid file descriptor specification
bash: line 11: read: 99999: invalid file descriptor: Bad file descriptor
bash: line 12: read: : invalid file descriptor specification
bash: line 14: read: 4: invalid file descriptor: Bad file descriptor
bash: line 16: read: 6: read error: Bad file descriptor

== merged 545
a=l1 b=l2
c=l3
status 1 d=[]
bash: line 8: read: 5: invalid file descriptor: Bad file descriptor
status 1
bash: line 9: read: abc: invalid file descriptor specification
status 1
bash: line 10: read: -1: invalid file descriptor specification
status 1
bash: line 11: read: 99999: invalid file descriptor: Bad file descriptor
status 1
bash: line 12: read: : invalid file descriptor specification
status 1
bash: line 14: read: 4: invalid file descriptor: Bad file descriptor
status 1
bash: line 16: read: 6: read error: Bad file descriptor
status 1

== status 0
