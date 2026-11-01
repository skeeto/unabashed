== stdout 97
st 127
trap2
st 127
st 126
st 126
st 127
st 0
no #! line: arg
st 0
still here 127
still here 127

== stderr 297
bash: line 3: /nonexistent/x: No such file or directory
bash: line 4: exec: nonexistentcmd: not found
bash: line 5: /tmp: Is a directory
bash: line 8: /nonexistent/y: No such file or directory
bash: line 13: /nonexistent/w: No such file or directory
bash: line 14: exec: nonexistentcmd: not found

== merged 394
bash: line 3: /nonexistent/x: No such file or directory
st 127
bash: line 4: exec: nonexistentcmd: not found
trap2
st 127
bash: line 5: /tmp: Is a directory
st 126
st 126
bash: line 8: /nonexistent/y: No such file or directory
st 127
st 0
no #! line: arg
st 0
bash: line 13: /nonexistent/w: No such file or directory
still here 127
bash: line 14: exec: nonexistentcmd: not found
still here 127

== status 0
