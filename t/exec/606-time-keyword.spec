== stdout 30
a
1
1
b
c
1
1
d
2
quiet
3
4
3

== stderr 256
bash: line 3: TIMEFORMAT: `Q': invalid format character
bash: line 4: TIMEFORMAT: `Q': invalid format character
bash: line 5: TIMEFORMAT: `Q': invalid format character
bash: line 6: TIMEFORMAT: `Q': invalid format character
timed %
timed %
timed %
timed %

== merged 286
a
bash: line 3: TIMEFORMAT: `Q': invalid format character
bash: line 4: TIMEFORMAT: `Q': invalid format character
1
bash: line 5: TIMEFORMAT: `Q': invalid format character
1
b
bash: line 6: TIMEFORMAT: `Q': invalid format character
c
timed %
timed %
1
timed %
1
d
timed %
2
quiet
3
4
3

== status 0
