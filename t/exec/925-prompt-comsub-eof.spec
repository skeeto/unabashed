== stdout 390
bash: command substitution: line 6: unexpected EOF while looking for matching `)'
[a]
st=0
[bare ]
bash: command substitution: line 8: unexpected EOF while looking for matching `)'
+: traced
bash: command substitution: line 8: unexpected EOF while looking for matching `)'
+set +x
t: bare
tset +x
bash: line 8: nope: bad
c${nope:?bad} : raw
bash: line 8: nope: bad
c${nope:?bad} set +x
end

== stderr 0

== merged 390
bash: command substitution: line 6: unexpected EOF while looking for matching `)'
[a]
st=0
[bare ]
bash: command substitution: line 8: unexpected EOF while looking for matching `)'
+: traced
bash: command substitution: line 8: unexpected EOF while looking for matching `)'
+set +x
t: bare
tset +x
bash: line 8: nope: bad
c${nope:?bad} : raw
bash: line 8: nope: bad
c${nope:?bad} set +x
end

== status 0
