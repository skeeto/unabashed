== stdout 65
dir st=126
noexec st=126
rel st=126
notdir st=126
command st=127

== stderr 252
bash: line 1: /tmp: Is a directory
bash: line 4: /tmp/notexec: Permission denied
bash: line 7: ./notexec: Permission denied
bash: line 9: /tmp/notexec/sub: Not a directory
bash: line 11: nosuch: command not found
bash: line 13: exec: nosuch: not found

== merged 317
bash: line 1: /tmp: Is a directory
dir st=126
bash: line 4: /tmp/notexec: Permission denied
noexec st=126
bash: line 7: ./notexec: Permission denied
rel st=126
bash: line 9: /tmp/notexec/sub: Not a directory
notdir st=126
bash: line 11: nosuch: command not found
command st=127
bash: line 13: exec: nosuch: not found

== status 127
