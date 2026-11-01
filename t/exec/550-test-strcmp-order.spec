== stdout 173
0 <- test  < a
1 <- test a < 
0 <- test A < a
0 <- test Z < a
0 <- test a < aa
0 <- test ab < b
0 <- test 10 < 9
0 <- test a > A
1 <- test same < same
1 <- test same > same

== stderr 378
bash: line 12: a: No such file or directory
bash: line 13: a: No such file or directory
bash: line 14: 9: No such file or directory
bash: line 1: [[: command not found
bash: line 16: same: No such file or directory
bash: line 1: [[: command not found
bash: line 19: abd: No such file or directory
bash: line 1: [[: command not found
bash: line 21: ~x: No such file or directory

== merged 551
0 <- test  < a
1 <- test a < 
0 <- test A < a
0 <- test Z < a
0 <- test a < aa
0 <- test ab < b
0 <- test 10 < 9
0 <- test a > A
1 <- test same < same
1 <- test same > same
bash: line 12: a: No such file or directory
bash: line 13: a: No such file or directory
bash: line 14: 9: No such file or directory
bash: line 1: [[: command not found
bash: line 16: same: No such file or directory
bash: line 1: [[: command not found
bash: line 19: abd: No such file or directory
bash: line 1: [[: command not found
bash: line 21: ~x: No such file or directory

== status 1
