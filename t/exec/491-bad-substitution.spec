== stdout 101
start
st=1
st=1
st=1
st=1
1
st=1
st=1
st=1
[abc]
after-h
st=0
[bash a b]
after-i
st=0

bang st=0
end

== stderr 308
bash: line 2: ${x!}: bad substitution
bash: line 3: ${:}: bad substitution
bash: line 4: ${x y}: bad substitution
bash: line 5: ${#x:1}: bad substitution
bash: line 6: ${1x}: bad substitution
bash: line 7: ${%}: bad substitution
bash: line 8: [${x:}]: bad substitution
bash: line 13: ${@:}: bad substitution

== merged 409
start
bash: line 2: ${x!}: bad substitution
st=1
bash: line 3: ${:}: bad substitution
st=1
bash: line 4: ${x y}: bad substitution
st=1
bash: line 5: ${#x:1}: bad substitution
st=1
1
bash: line 6: ${1x}: bad substitution
st=1
bash: line 7: ${%}: bad substitution
st=1
bash: line 8: [${x:}]: bad substitution
st=1
[abc]
after-h
st=0
[bash a b]
after-i
st=0

bang st=0
bash: line 13: ${@:}: bad substitution
end

== status 0
