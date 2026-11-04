== stdout 101
C:/work
C:/work
C:/Windows/System32
C:/Windows
C:/WORK
D:/
status 1
D:/
status 1
C:/work/sub
C:/work

== stderr 167
bash: line 8: cd: /c/Windows: No such file or directory
bash: line 13: cd: E:/: No such file or directory
bash: line 16: cd: C:/work/nosuch: No such file or directory

== merged 268
C:/work
C:/work
bash: line 8: cd: /c/Windows: No such file or directory
C:/Windows/System32
C:/Windows
C:/WORK
D:/
bash: line 13: cd: E:/: No such file or directory
status 1
D:/
bash: line 16: cd: C:/work/nosuch: No such file or directory
status 1
C:/work/sub
C:/work

== status 0
