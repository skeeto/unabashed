== stdout 131
PWD gone
pwd 1
redirection failed
*
/
/
/
/
home
status 0
PWD 5028
pwd same
hello
x
f2 file
cat: file: File name too long
status 1

== stderr 379
shell-init: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
bash: line 4: f: No such file or directory
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory

== merged 510
shell-init: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
PWD gone
pwd: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
pwd 1
bash: line 4: f: No such file or directory
redirection failed
*
/
/
/
chdir: error retrieving current directory: getcwd: cannot access parent directories: No such file or directory
/
home
status 0
PWD 5028
pwd same
hello
x
f2 file
cat: file: File name too long
status 1

== status 0
