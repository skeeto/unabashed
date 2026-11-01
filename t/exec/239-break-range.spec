== stdout 73
1a
break 0: 1
continue -1: 1
break -- 0: 1
break -- 2: 0 1
1
2
spaces: 0

== stderr 150
bash: line 5: break: 0: loop count out of range
bash: line 13: continue: -1: loop count out of range
bash: line 18: break: 0: loop count out of range

== merged 223
1a
bash: line 5: break: 0: loop count out of range
break 0: 1
bash: line 13: continue: -1: loop count out of range
continue -1: 1
bash: line 18: break: 0: loop count out of range
break -- 0: 1
break -- 2: 0 1
1
2
spaces: 0

== status 0
