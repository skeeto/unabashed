== stdout 327
ERR at 7
next 9
1 10
13 13
for 1
for 16
case 19
2 20
f 24 22 24 NULL 23
g 26 26
ERR at 32
declare -a FUNCNAME=([0]="k") declare -a BASH_LINENO=([0]="34")
declare -a FUNCNAME=([0]="k" [1]="h") declare -a BASH_LINENO=([0]="35" [1]="36")
declare -a FUNCNAME=([0]="h")
declare -a FUNCNAME declare -a BASH_LINENO=()
38
ERR at 39
41

== stderr 84
bash: line 39: /nonexistent/x: No such file or directory
bash: line 42: nosuch: err

== merged 411
ERR at 7
next 9
1 10
13 13
for 1
for 16
case 19
2 20
f 24 22 24 NULL 23
g 26 26
ERR at 32
declare -a FUNCNAME=([0]="k") declare -a BASH_LINENO=([0]="34")
declare -a FUNCNAME=([0]="k" [1]="h") declare -a BASH_LINENO=([0]="35" [1]="36")
declare -a FUNCNAME=([0]="h")
declare -a FUNCNAME declare -a BASH_LINENO=()
38
bash: line 39: /nonexistent/x: No such file or directory
ERR at 39
41
bash: line 42: nosuch: err

== status 127
