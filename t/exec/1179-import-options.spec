== stdout 871
1
f () { echo f; }
g () {  echo g
}
f; g
f
g
2
f () { echo f; }
h () { echo h; }; echo inj
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
f
f
3
f () { echo f; }
:
4
+ :
5
f () { echo f; }
:
6
f
f
7
/usr/local/bin/bash: line 0: /usr/local/bin/bash: nonexist: invalid option name
8
/usr/local/bin/bash: error importing function definition for `a-b'
f
/usr/local/bin/bash: line 1: a-b: command not found
127
9
/usr/local/bin/bash: error importing function definition for `a-b'
/usr/local/bin/bash: line 1: a-b: command not found
127
10
/usr/local/bin/bash: error importing function definition for `a-b'
/usr/local/bin/bash: line 1: a-b: command not found
127
11
ab
0
12
ab
0
13
/usr/local/bin/bash: error importing function definition for `a-b'
() { echo ab; }
14
() { echo f; }
f
15
f

== stderr 0

== merged 871
1
f () { echo f; }
g () {  echo g
}
f; g
f
g
2
f () { echo f; }
h () { echo h; }; echo inj
/usr/local/bin/bash: warning: h: ignoring function definition attempt
/usr/local/bin/bash: error importing function definition for `h'
f
f
3
f () { echo f; }
:
4
+ :
5
f () { echo f; }
:
6
f
f
7
/usr/local/bin/bash: line 0: /usr/local/bin/bash: nonexist: invalid option name
8
/usr/local/bin/bash: error importing function definition for `a-b'
f
/usr/local/bin/bash: line 1: a-b: command not found
127
9
/usr/local/bin/bash: error importing function definition for `a-b'
/usr/local/bin/bash: line 1: a-b: command not found
127
10
/usr/local/bin/bash: error importing function definition for `a-b'
/usr/local/bin/bash: line 1: a-b: command not found
127
11
ab
0
12
ab
0
13
/usr/local/bin/bash: error importing function definition for `a-b'
() { echo ab; }
14
() { echo f; }
f
15
f

== status 0
