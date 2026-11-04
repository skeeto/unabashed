== stdout 279
st=127
st=126
st=126
restored

kept
renamed
st=1
st=126
handler: 3 nothere a b
3 0
127
1
0
sub: 1
group: 1
st=126
st=126
st=127
sourced
body
sourced
expanded
dquote
squote
procsub
glob
backslash
sourced
cmdsub
sourced
param
empty
sourced
tilde
notilde
home
dir
st=0
missing
st=0

== stderr 447
bash: line 3: exec: nothere: not found
bash: line 4: exec: dir: cannot execute: Is a directory
bash: line 5: /home/oracle/dir: Is a directory
bash: line 6: .: Is a directory
bash: line 1: exec: dir: cannot execute: Is a directory
bash: line 17: nothere: command not found
./bin: ./bin: cannot execute binary file
dir: dir: Is a directory
./bin: line 1: $'\177ELF': command not found
bash: /home/oracle: is a directory
bash: envdir: is a directory

== merged 726
bash: line 3: exec: nothere: not found
st=127
bash: line 4: exec: dir: cannot execute: Is a directory
st=126
bash: line 5: /home/oracle/dir: Is a directory
st=126
bash: line 6: .: Is a directory
restored

kept
renamed
st=1
bash: line 1: exec: dir: cannot execute: Is a directory
st=126
handler: 3 nothere a b
3 0
bash: line 17: nothere: command not found
127
1
0
sub: 1
group: 1
./bin: ./bin: cannot execute binary file
st=126
dir: dir: Is a directory
st=126
./bin: line 1: $'\177ELF': command not found
st=127
sourced
body
sourced
expanded
dquote
squote
procsub
glob
backslash
sourced
cmdsub
sourced
param
empty
sourced
tilde
notilde
bash: /home/oracle: is a directory
home
bash: envdir: is a directory
dir
st=0
missing
st=0

== status 0
