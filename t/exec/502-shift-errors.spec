== stdout 114
a st=1 n=3
b st=1 n=3
c st=2 n=3
e st=0 n=0
f st=1 n=0
g st=1
h st=1 n=1
i st=0 n=1
in f st=1 n=2
dd st=0 n=1
end

== stderr 249
bash: line 4: shift: -1: shift count out of range
bash: line 6: shift: abc: numeric argument required
bash: line 13: shift: shift count out of range
bash: line 16: shift: 2: shift count out of range
bash: line 20: shift: 3: shift count out of range

== merged 363
a st=1 n=3
bash: line 4: shift: -1: shift count out of range
b st=1 n=3
bash: line 6: shift: abc: numeric argument required
c st=2 n=3
e st=0 n=0
f st=1 n=0
bash: line 13: shift: shift count out of range
g st=1
bash: line 16: shift: 2: shift count out of range
h st=1 n=1
i st=0 n=1
bash: line 20: shift: 3: shift count out of range
in f st=1 n=2
dd st=0 n=1
end

== status 0
