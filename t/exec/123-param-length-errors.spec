== stdout 39
status 1
status 1
0
after
status 0
end

== stderr 81
bash: line 2: ${#x:-a}: bad substitution
bash: line 3: ${#x#a}: bad substitution

== merged 120
bash: line 2: ${#x:-a}: bad substitution
status 1
bash: line 3: ${#x#a}: bad substitution
status 1
0
after
status 0
end

== status 0
