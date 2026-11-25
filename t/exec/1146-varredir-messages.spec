== stdout 40
fd=10
hi
g=11
hi
k=12
five
status 1 x=3

== stderr 1488
bash: line 7: fd: ambiguous redirect
bash: line 8: fd: ambiguous redirect
bash: line 9: fd: ambiguous redirect
bash: line 10: fd: ambiguous redirect
bash: line 11: fd: ambiguous redirect
bash: line 12: fd: ambiguous redirect
bash: line 13: fd: ambiguous redirect
bash: line 14: fd: ambiguous redirect
bash: line 15: fd: ambiguous redirect
bash: line 16: fd: ambiguous redirect
bash: line 17: /nope/x: No such file or directory
bash: line 18: $u: ambiguous redirect
bash: line 19: fd: ambiguous redirect
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 20: 7: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 21: 7: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 22: 7: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 23: 7: Bad file descriptor
bash: line 24: 7: Bad file descriptor
bash: line 25: fd: cannot overwrite existing file
bash: line 25: ex: cannot overwrite existing file
bash: line 1: fd: restricted: cannot redirect output
bash: line 1: fd: restricted: cannot redirect output
bash: line 1: fd: restricted: cannot redirect output
bash: line 1: f: restricted: cannot redirect output
bash: line 27: 7: Bad file descriptor
bash: line 28: 8: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 30: 4: Bad file descriptor
bash: line 31: $fd: Bad file descriptor

== merged 1528
bash: line 7: fd: ambiguous redirect
bash: line 8: fd: ambiguous redirect
bash: line 9: fd: ambiguous redirect
bash: line 10: fd: ambiguous redirect
bash: line 11: fd: ambiguous redirect
bash: line 12: fd: ambiguous redirect
bash: line 13: fd: ambiguous redirect
bash: line 14: fd: ambiguous redirect
bash: line 15: fd: ambiguous redirect
bash: line 16: fd: ambiguous redirect
bash: line 17: /nope/x: No such file or directory
bash: line 18: $u: ambiguous redirect
bash: line 19: fd: ambiguous redirect
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 20: 7: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 21: 7: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 22: 7: Bad file descriptor
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 23: 7: Bad file descriptor
bash: line 24: 7: Bad file descriptor
bash: line 25: fd: cannot overwrite existing file
bash: line 25: ex: cannot overwrite existing file
bash: line 1: fd: restricted: cannot redirect output
bash: line 1: fd: restricted: cannot redirect output
bash: line 1: fd: restricted: cannot redirect output
bash: line 1: f: restricted: cannot redirect output
fd=10
hi
bash: line 27: 7: Bad file descriptor
g=11
hi
bash: line 28: 8: Bad file descriptor
k=12
five
bash: redirection error: cannot duplicate fd: Bad file descriptor
bash: line 30: 4: Bad file descriptor
status 1 x=3
bash: line 31: $fd: Bad file descriptor

== status 1
