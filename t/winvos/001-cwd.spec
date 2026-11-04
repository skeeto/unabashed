== stdout 113
C:/work
C:/work
/c/Windows
C:/Windows/System32
C:/\Windows
c:/WORK
D:/
status 1
D:/
status 1
C:/work/sub
C:/work

== stderr 111
bash: line 12: cd: E:/: No such file or directory
bash: line 15: cd: C:/work/nosuch: No such file or directory

== merged 224
C:/work
C:/work
/c/Windows
C:/Windows/System32
C:/\Windows
c:/WORK
D:/
bash: line 12: cd: E:/: No such file or directory
status 1
D:/
bash: line 15: cd: C:/work/nosuch: No such file or directory
status 1
C:/work/sub
C:/work

== status 0
