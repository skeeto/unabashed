== stdout 313
L=2 S=environment F=f BL=1
1 NULL
L=3
st=0
L=2 S=environment bash F=f k BL=1 2
f 0 environment
g 0 environment
h 0 environment
+1: h
+0: declare -p nonex
environment: line 0: declare: nonex: not found
environment: line 1: `arr[x]': not a valid identifier
+1: true
+2: caller
1 NULL
+3: echo L=3
L=3
S=environment

== stderr 504
environment: nosuch: command not found
environment: line 1: nope: No such file or directory
environment: line 3: x: unset here
environment: 1/0: division by 0 (error token is "0")
environment: line 0: declare: nonex: not found
environment: line 1: `arr[x]': not a valid identifier
environment: nosuch: command not found
environment: line 1: nope: No such file or directory
environment: line 3: x: unset here
environment: line 2: nosuch2: command not found
environment: line 2: nosuch2: command not found

== merged 817
environment: nosuch: command not found
environment: line 1: nope: No such file or directory
L=2 S=environment F=f BL=1
environment: line 3: x: unset here
environment: 1/0: division by 0 (error token is "0")
environment: line 0: declare: nonex: not found
environment: line 1: `arr[x]': not a valid identifier
1 NULL
L=3
st=0
environment: nosuch: command not found
environment: line 1: nope: No such file or directory
L=2 S=environment bash F=f k BL=1 2
environment: line 3: x: unset here
f 0 environment
g 0 environment
h 0 environment
+1: h
+0: declare -p nonex
environment: line 0: declare: nonex: not found
environment: line 1: `arr[x]': not a valid identifier
+1: true
+2: caller
1 NULL
+3: echo L=3
L=3
environment: line 2: nosuch2: command not found
S=environment
environment: line 2: nosuch2: command not found

== status 127
