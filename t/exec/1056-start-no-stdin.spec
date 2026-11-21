== stdout 154
dup 1
same 0
read 1
cat 1
tty 1
fd 1
pipeline 1 0
subshell 1
string
herestring 0
heredoc
heredoc 0
cmdsub inner
null 0
reopened 0
closed again 1
status 0

== stderr 232
bash: line 2: 0: Bad file descriptor
bash: line 4: read: 0: read error: Bad file descriptor
cat: -: Bad file descriptor
cat: -: Bad file descriptor
cat: -: Bad file descriptor
bash: line 18: read: 0: read error: Bad file descriptor

== merged 386
bash: line 2: 0: Bad file descriptor
dup 1
same 0
bash: line 4: read: 0: read error: Bad file descriptor
read 1
cat: -: Bad file descriptor
cat 1
tty 1
fd 1
cat: -: Bad file descriptor
pipeline 1 0
cat: -: Bad file descriptor
subshell 1
string
herestring 0
heredoc
heredoc 0
cmdsub inner
null 0
reopened 0
bash: line 18: read: 0: read error: Bad file descriptor
closed again 1
status 0

== status 0
