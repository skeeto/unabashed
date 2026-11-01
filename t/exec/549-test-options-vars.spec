== stdout 345
1 <- test -o noglob
0 <- test -o noglob
1 <- test -o errexit
1 <- test -o nosuchopt
0 <- test ! -o noglob
0 <- test -v x
0 <- test -v e
1 <- test -v nosuch
1 <- test -v x
0 <- test -v 1
0 <- test -v 2
0 <- test -v 3
0 <- test -v 0
1 <- test -v #
1 <- test -v ?
127 <- [[ -v e ]]
127 <- [[ -v 2 ]]
127 <- [[ -o noglob ]]
127 <- [[ ! -v nosuch ]]

== stderr 144
bash: line 1: [[: command not found
bash: line 1: [[: command not found
bash: line 1: [[: command not found
bash: line 1: [[: command not found

== merged 489
1 <- test -o noglob
0 <- test -o noglob
1 <- test -o errexit
1 <- test -o nosuchopt
0 <- test ! -o noglob
0 <- test -v x
0 <- test -v e
1 <- test -v nosuch
1 <- test -v x
0 <- test -v 1
0 <- test -v 2
0 <- test -v 3
0 <- test -v 0
1 <- test -v #
1 <- test -v ?
bash: line 1: [[: command not found
127 <- [[ -v e ]]
bash: line 1: [[: command not found
127 <- [[ -v 2 ]]
bash: line 1: [[: command not found
127 <- [[ -o noglob ]]
bash: line 1: [[: command not found
127 <- [[ ! -v nosuch ]]

== status 0
