== stdout 109
a st=1 r=1
prefix
b st=0
st=1
d st=1 r=1
e st=1
f st=1
g st=1
h st=1
in f 1
i st=0
j st=1 r=1
k st=1 r=1
end

== stderr 428
bash: line 2: r: readonly variable
bash: line 4: r: readonly variable
bash: line 6: r: readonly variable
bash: line 7: unset: r: cannot unset: readonly variable
bash: line 9: declare: r: readonly variable
bash: line 11: r: readonly variable
bash: line 13: r: readonly variable
bash: line 15: r: readonly variable
bash: line 17: local: r: readonly variable
bash: line 20: r: readonly variable
bash: line 22: r: readonly variable

== merged 537
bash: line 2: r: readonly variable
a st=1 r=1
bash: line 4: r: readonly variable
prefix
b st=0
bash: line 6: r: readonly variable
st=1
bash: line 7: unset: r: cannot unset: readonly variable
d st=1 r=1
bash: line 9: declare: r: readonly variable
e st=1
bash: line 11: r: readonly variable
f st=1
bash: line 13: r: readonly variable
g st=1
bash: line 15: r: readonly variable
h st=1
bash: line 17: local: r: readonly variable
in f 1
i st=0
bash: line 20: r: readonly variable
j st=1 r=1
bash: line 22: r: readonly variable
k st=1 r=1
end

== status 0
