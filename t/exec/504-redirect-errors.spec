== stdout 86
a st=1
b st=1
c st=1
d st=1
e st=1
f st=1 x=1
g st=1
h st=1
i st=0
l=three
j st=1
end

== stderr 470
bash: line 1: /nonexistent/f: No such file or directory
bash: line 3: /nonexistent/in: No such file or directory
bash: line 5: /tmp: Is a directory
bash: line 7: /nonexistent/g: No such file or directory
bash: line 10: /nonexistent/h: No such file or directory
bash: line 12: /nonexistent/i: No such file or directory
bash: line 14: 9: Bad file descriptor
bash: line 18: /tmp/clob: cannot overwrite existing file
bash: line 23: /nonexistent/j: No such file or directory

== merged 556
bash: line 1: /nonexistent/f: No such file or directory
a st=1
bash: line 3: /nonexistent/in: No such file or directory
b st=1
bash: line 5: /tmp: Is a directory
c st=1
bash: line 7: /nonexistent/g: No such file or directory
d st=1
bash: line 10: /nonexistent/h: No such file or directory
e st=1
bash: line 12: /nonexistent/i: No such file or directory
f st=1 x=1
bash: line 14: 9: Bad file descriptor
g st=1
bash: line 18: /tmp/clob: cannot overwrite existing file
h st=1
i st=0
l=three
bash: line 23: /nonexistent/j: No such file or directory
j st=1
end

== status 0
