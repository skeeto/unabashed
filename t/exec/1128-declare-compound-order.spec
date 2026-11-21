== stdout 605
a 1 1
declare -a fe2=([0]="1" [1]="2")
declare -a fe3=()
declare -a z1=([0]="x")
b 0
declare -ar ro3=([0]="b")
c 1
declare -a ia2=([0]="1")
d 0
declare -n r3="t3"
declare -a t3=([0]="1" [1]="2")
e 1
declare -a r4=([0]="1")
f 1
declare -a la=([0]="1" [1]="2")
declare -ai ii=([0]="2")
declare -ai jj=([0]="4")
declare -ai n2=([0]="2+2")
declare -au up=([0]="ABC")
declare -au up2=([0]="DEF")
fn: da
g 0
declare -a da=([0]="1" [1]="2")
bash: line 29: declare: fl: not found
declare -ai fl=([0]="7")
in 2 3
out unset 3
declare -n ref="var"
bash: line 33: declare: var: not found
declare -a foo=([0]="X")
end

== stderr 603
bash: line 8: /nonexistent/x: No such file or directory
+ fe2=('1' '2')
+ declare fe2
bash: line 11: /nonexistent/x: No such file or directory
+ fe3=()
+ X=1
+ declare fe3
bash: line 12: /nonexistent/x: No such file or directory
+ set +x
one
two
bash: line 18: declare: ia2: cannot destroy array variables in this way
bash: line 20: declare: r4: reference variable cannot be an array
bash: line 21: local: can only be used in a function
bash: line 24: declare: `+u': not a valid identifier
bash: line 25: a[1]: cannot assign list to array member
bash: line 29: /nonexistent/x: No such file or directory

== merged 1208
bash: line 8: /nonexistent/x: No such file or directory
a 1 1
+ fe2=('1' '2')
+ declare fe2
bash: line 11: /nonexistent/x: No such file or directory
+ fe3=()
+ X=1
+ declare fe3
bash: line 12: /nonexistent/x: No such file or directory
+ set +x
declare -a fe2=([0]="1" [1]="2")
declare -a fe3=()
declare -a z1=([0]="x")
one
two
b 0
declare -ar ro3=([0]="b")
bash: line 18: declare: ia2: cannot destroy array variables in this way
c 1
declare -a ia2=([0]="1")
d 0
declare -n r3="t3"
declare -a t3=([0]="1" [1]="2")
bash: line 20: declare: r4: reference variable cannot be an array
e 1
declare -a r4=([0]="1")
bash: line 21: local: can only be used in a function
f 1
declare -a la=([0]="1" [1]="2")
declare -ai ii=([0]="2")
declare -ai jj=([0]="4")
declare -ai n2=([0]="2+2")
bash: line 24: declare: `+u': not a valid identifier
declare -au up=([0]="ABC")
declare -au up2=([0]="DEF")
bash: line 25: a[1]: cannot assign list to array member
fn: da
g 0
declare -a da=([0]="1" [1]="2")
bash: line 29: declare: fl: not found
bash: line 29: /nonexistent/x: No such file or directory
declare -ai fl=([0]="7")
in 2 3
out unset 3
declare -n ref="var"
bash: line 33: declare: var: not found
declare -a foo=([0]="X")
end

== status 0
