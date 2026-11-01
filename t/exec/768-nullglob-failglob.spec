== stdout 95
2: a1 a2
2 a1 a2
empty for done
[y]
a1 a2
before
in group
status 1
after f 1
comsub 1 []
for 1

== stderr 196
bash: line 16: no match: z*
bash: line 17: no match: z*
bash: line 18: no match: z*
bash: line 20: no match: z*
bash: line 23: no match: z*
bash: line 25: no match: z*
bash: line 28: no match: z*

== merged 291
2: a1 a2
2 a1 a2
empty for done
[y]
a1 a2
before
bash: line 16: no match: z*
bash: line 17: no match: z*
in group
bash: line 18: no match: z*
status 1
bash: line 20: no match: z*
after f 1
bash: line 23: no match: z*
comsub 1 []
bash: line 25: no match: z*
for 1
bash: line 28: no match: z*

== status 1
