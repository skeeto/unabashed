== stdout 196
1 127
2 127
3 0
hash: hash table empty
4 0
5 127
6 0 /usr/local/bin:/opt/fakes/bin
h nosuch x=1 p=/usr/local/bin:/opt/fakes/bin
7 0
h cat x=2 p=/nowhere
8 0
h cat x= p=/nowhere
9 0
3
10 0 x=
11 0

== stderr 185
bash: line 4: cat: command not found
bash: line 5: cat: command not found
bash: line 9: cat: command not found
bash: line 11: cat: command not found
bash: line 19: x: readonly variable

== merged 381
bash: line 4: cat: command not found
1 127
bash: line 5: cat: command not found
2 127
3 0
hash: hash table empty
4 0
bash: line 9: cat: command not found
5 127
bash: line 11: cat: command not found
6 0 /usr/local/bin:/opt/fakes/bin
h nosuch x=1 p=/usr/local/bin:/opt/fakes/bin
7 0
h cat x=2 p=/nowhere
8 0
h cat x= p=/nowhere
9 0
3
10 0 x=
bash: line 19: x: readonly variable
11 0

== status 0
