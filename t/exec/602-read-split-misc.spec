== stdout 129
-a  b-
-a-b c-
-a -
[  line][b ]
[a][b]
2
2 a b
IFS=[,] var=[abc] rest=[def,ghi]
IFS=[ ] var=[  abc] rest=[def,ghi ]
1
1
2
1
1
2

== stderr 230
bash: line 18: r: readonly variable
bash: line 19: r: readonly variable
bash: line 20: r: readonly variable
bash: line 21: r: readonly variable
bash: line 22: read: as: not an indexed array
bash: line 23: REPLY: readonly variable

== merged 359
-a  b-
-a-b c-
-a -
[  line][b ]
[a][b]
2
2 a b
IFS=[,] var=[abc] rest=[def,ghi]
IFS=[ ] var=[  abc] rest=[def,ghi ]
bash: line 18: r: readonly variable
1
bash: line 19: r: readonly variable
1
bash: line 20: r: readonly variable
2
bash: line 21: r: readonly variable
1
bash: line 22: read: as: not an indexed array
1
bash: line 23: REPLY: readonly variable
2

== status 0
