== stdout 735
hash: hash table empty
hits	command
   2	/opt/fakes/bin/cat
cat is hashed (/opt/fakes/bin/cat)
file
/opt/fakes/bin/cat
/opt/fakes/bin/cat
/opt/fakes/bin/cat
cat is hashed (/opt/fakes/bin/cat)
hits	command
   8	/opt/fakes/bin/cat
/opt/fakes/bin/cat
1
builtin hash -p /opt/fakes/bin/cat cat
ls	/opt/fakes/bin/ls
ls	/opt/fakes/bin/ls
hits	command
   9	/opt/fakes/bin/cat
1
127
foo is hashed (/nonexist/foo)
/nonexist/foo
/nonexist/foo
hash: hash table empty
hits	command
   4	/nonexist/foo
   9	/opt/fakes/bin/cat
hash: hash table empty
hits	command
   0	/tmp/x
   1	/opt/fakes/bin/cat
hi
0
2
hits	command
   0	/tmp/x
   0	/bin/sh
   0	/tmp/bash
   1	/opt/fakes/bin/cat
1
2
./e
e is ./e
ran e
hits	command
   1	./e
found in cwd
ran e
127

== stderr 405
bash: line 6: hash: nope: not found
bash: line 10: hash: ls: not found
bash: line 11: /nonexist/foo: No such file or directory
bash: line 12: type: foo: not found
hash: usage: hash [-lr] [-p pathname] [-dt] [name ...]
bash: line 20: hash: nonexist: not found
bash: line 21: hash: -z: invalid option
hash: usage: hash [-lr] [-p pathname] [-dt] [name ...]
bash: line 24: nonexist: No such file or directory

== merged 1140
hash: hash table empty
hits	command
   2	/opt/fakes/bin/cat
cat is hashed (/opt/fakes/bin/cat)
file
/opt/fakes/bin/cat
/opt/fakes/bin/cat
/opt/fakes/bin/cat
cat is hashed (/opt/fakes/bin/cat)
hits	command
   8	/opt/fakes/bin/cat
/opt/fakes/bin/cat
bash: line 6: hash: nope: not found
1
builtin hash -p /opt/fakes/bin/cat cat
ls	/opt/fakes/bin/ls
ls	/opt/fakes/bin/ls
hits	command
   9	/opt/fakes/bin/cat
bash: line 10: hash: ls: not found
1
bash: line 11: /nonexist/foo: No such file or directory
127
foo is hashed (/nonexist/foo)
/nonexist/foo
bash: line 12: type: foo: not found
/nonexist/foo
hash: hash table empty
hits	command
   4	/nonexist/foo
   9	/opt/fakes/bin/cat
hash: hash table empty
hits	command
   0	/tmp/x
   1	/opt/fakes/bin/cat
hi
0
hash: usage: hash [-lr] [-p pathname] [-dt] [name ...]
2
hits	command
   0	/tmp/x
   0	/bin/sh
   0	/tmp/bash
   1	/opt/fakes/bin/cat
bash: line 20: hash: nonexist: not found
1
bash: line 21: hash: -z: invalid option
hash: usage: hash [-lr] [-p pathname] [-dt] [name ...]
2
./e
e is ./e
ran e
hits	command
   1	./e
found in cwd
ran e
bash: line 24: nonexist: No such file or directory
127

== status 0
