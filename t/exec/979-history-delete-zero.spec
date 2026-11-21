== stdout 500
status 1
status 1
    1  b
    2  c
    3  d
-0: status 1
    1  a
    2  b
    3  c
0: status 1
    1  a
    2  b
    3  c
4: status 1
    1  a
    2  b
    3  c
-4: status 1
    1  a
    2  b
    3  c
-00: status 1
    1  a
    2  b
    3  c
+0: status 1
    1  a
    2  b
    3  c
-1--0: status 1
    1  a
    2  b
    3  c
1-0: status 0
    1  b
    2  c
-0-1: status 0
    1  b
    2  c
-0--0: status 0
    1  b
    2  c
-3-3: status 0
2-1: status 1
    1  a
    2  b
    3  c
status 1
status 1

== stderr 576
bash: line 4: history: -0: history position out of range
bash: line 9: history: -0: history position out of range
bash: line 15: history: -0: history position out of range
bash: line 15: history: 0: history position out of range
bash: line 15: history: 4: history position out of range
bash: line 15: history: -4: history position out of range
bash: line 15: history: -00: history position out of range
bash: line 15: history: +0: history position out of range
bash: line 19: history: 1: history position out of range
bash: line 20: history: -0: history position out of range

== merged 1076
bash: line 4: history: -0: history position out of range
status 1
bash: line 9: history: -0: history position out of range
status 1
    1  b
    2  c
    3  d
bash: line 15: history: -0: history position out of range
-0: status 1
    1  a
    2  b
    3  c
bash: line 15: history: 0: history position out of range
0: status 1
    1  a
    2  b
    3  c
bash: line 15: history: 4: history position out of range
4: status 1
    1  a
    2  b
    3  c
bash: line 15: history: -4: history position out of range
-4: status 1
    1  a
    2  b
    3  c
bash: line 15: history: -00: history position out of range
-00: status 1
    1  a
    2  b
    3  c
bash: line 15: history: +0: history position out of range
+0: status 1
    1  a
    2  b
    3  c
-1--0: status 1
    1  a
    2  b
    3  c
1-0: status 0
    1  b
    2  c
-0-1: status 0
    1  b
    2  c
-0--0: status 0
    1  b
    2  c
-3-3: status 0
2-1: status 1
    1  a
    2  b
    3  c
bash: line 19: history: 1: history position out of range
status 1
bash: line 20: history: -0: history position out of range
status 1

== status 0
