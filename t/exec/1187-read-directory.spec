== stdout 915
-- read x <.; echo $? "<$x>"
bash: line 1: read: 0: read error: Is a directory
1 <>
status 0
-- read -r -n 3 x <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -d '' x </; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -t 1 x <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -t 0 x <.; echo $?
0
status 0
-- exec 3<.; read -u 3 x; echo $?
bash: line 1: read: 3: read error: Is a directory
1
status 0
-- while read l; do :; done <.; echo $?
bash: line 1: read: 0: read error: Is a directory
0
status 0
-- read -a arr <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -N 1 x <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read x 0>/dev/null; echo $?
bash: line 1: read: 0: read error: Bad file descriptor
1
status 0
-- mapfile a <.; echo $? ${#a[@]}
0 0
status 0

== stderr 0

== merged 915
-- read x <.; echo $? "<$x>"
bash: line 1: read: 0: read error: Is a directory
1 <>
status 0
-- read -r -n 3 x <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -d '' x </; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -t 1 x <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -t 0 x <.; echo $?
0
status 0
-- exec 3<.; read -u 3 x; echo $?
bash: line 1: read: 3: read error: Is a directory
1
status 0
-- while read l; do :; done <.; echo $?
bash: line 1: read: 0: read error: Is a directory
0
status 0
-- read -a arr <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read -N 1 x <.; echo $?
bash: line 1: read: 0: read error: Is a directory
1
status 0
-- read x 0>/dev/null; echo $?
bash: line 1: read: 0: read error: Bad file descriptor
1
status 0
-- mapfile a <.; echo $? ${#a[@]}
0 0
status 0

== status 0
