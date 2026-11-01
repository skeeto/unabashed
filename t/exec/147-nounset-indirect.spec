== stdout 45
status 1
[def]
after
status 0
status 1
t
end

== stderr 86
bash: line 4: !r: unbound variable
bash: line 6: unsetref: invalid indirect expansion

== merged 131
bash: line 4: !r: unbound variable
status 1
[def]
after
status 0
bash: line 6: unsetref: invalid indirect expansion
status 1
t
end

== status 0
