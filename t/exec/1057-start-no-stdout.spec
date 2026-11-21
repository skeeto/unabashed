== stdout 125
status 0
missing 127
dup 1
echo 1
loop 127
trapped 1
program 1
bash: line 9: nosuch: command not found
status 0
all closed 5

== stderr 312
bash: line 2: echo: write error: Bad file descriptor
echo 1
bash: line 3: printf: write error: Bad file descriptor
printf 1
bash: line 4: 1: Bad file descriptor
dup 1
cat: write error: Bad file descriptor
pipeline 0 1
bash: line 6: echo: write error: Bad file descriptor
subshell 1
cmdsub inner
tty 1
reopened 0

== merged 437
bash: line 2: echo: write error: Bad file descriptor
echo 1
bash: line 3: printf: write error: Bad file descriptor
printf 1
bash: line 4: 1: Bad file descriptor
dup 1
cat: write error: Bad file descriptor
pipeline 0 1
bash: line 6: echo: write error: Bad file descriptor
subshell 1
cmdsub inner
tty 1
reopened 0
status 0
missing 127
dup 1
echo 1
loop 127
trapped 1
program 1
bash: line 9: nosuch: command not found
status 0
all closed 5

== status 0
