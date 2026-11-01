== stdout 62
a
status 0
status 1
status 1
1
status 0
status 2
status 2
end

== stderr 159
bash: line 4: $2: cannot assign in this way
bash: line 5: $2: cannot assign in this way
bash: line 7: r: readonly variable
bash: line 8: r2: readonly variable

== merged 221
a
status 0
bash: line 4: $2: cannot assign in this way
status 1
bash: line 5: $2: cannot assign in this way
status 1
1
status 0
bash: line 7: r: readonly variable
status 2
bash: line 8: r2: readonly variable
status 2
end

== status 0
