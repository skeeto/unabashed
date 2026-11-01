== stdout 40
status 1
status 1
status 1
status 1
end

== stderr 156
bash: line 3: ${x@Z}: bad substitution
bash: line 4: ${x@}: bad substitution
bash: line 5: ${x@QQ}: bad substitution
bash: line 6: ${x@q}: bad substitution

== merged 196
bash: line 3: ${x@Z}: bad substitution
status 1
bash: line 4: ${x@}: bad substitution
status 1
bash: line 5: ${x@QQ}: bad substitution
status 1
bash: line 6: ${x@q}: bad substitution
status 1
end

== status 0
