== stdout 345
status 1
bash: line 4: /tmp/nope: No such file or directory
status 1
bash: line 6: source: filename argument required
source: usage: source [-p path] filename [arguments]
no arg: 2
bash: line 8: .: filename argument required
.: usage: . [-p path] filename [arguments]
dot no arg: 2
bash: line 10: source: /tmp: is a directory
dir: 1
still alive

== stderr 61
bash: line 2: /tmp/nonexistent.sh: No such file or directory

== merged 406
bash: line 2: /tmp/nonexistent.sh: No such file or directory
status 1
bash: line 4: /tmp/nope: No such file or directory
status 1
bash: line 6: source: filename argument required
source: usage: source [-p path] filename [arguments]
no arg: 2
bash: line 8: .: filename argument required
.: usage: . [-p path] filename [arguments]
dot no arg: 2
bash: line 10: source: /tmp: is a directory
dir: 1
still alive

== status 0
