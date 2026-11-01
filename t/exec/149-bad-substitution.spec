== stdout 99
status 1
status 1
status 1
status 1
status 1
status 1
status 1

after
status 0

after
status 0
end

== stderr 273
bash: line 2: ${x;}: bad substitution
bash: line 3: ${x!}: bad substitution
bash: line 4: ${1a}: bad substitution
bash: line 5: ${x:}: bad substitution
bash: line 6: ${-x}: bad substitution
bash: line 7: ${x$}: bad substitution
bash: line 8: ${a-b}${c.d}: bad substitution

== merged 372
bash: line 2: ${x;}: bad substitution
status 1
bash: line 3: ${x!}: bad substitution
status 1
bash: line 4: ${1a}: bad substitution
status 1
bash: line 5: ${x:}: bad substitution
status 1
bash: line 6: ${-x}: bad substitution
status 1
bash: line 7: ${x$}: bad substitution
status 1
bash: line 8: ${a-b}${c.d}: bad substitution
status 1

after
status 0

after
status 0
end

== status 0
