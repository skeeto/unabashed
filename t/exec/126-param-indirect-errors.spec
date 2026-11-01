== stdout 58
status 1
status 1
status 1
status 1
status 1
status 1
end

== stderr 250
bash: line 3: u: invalid indirect expansion
bash: line 4: a b: invalid variable name
bash: line 5: x-y: invalid variable name
bash: line 6: : invalid variable name
bash: line 7: u: invalid indirect expansion
bash: line 8: x:-y: invalid variable name

== merged 308
bash: line 3: u: invalid indirect expansion
status 1
bash: line 4: a b: invalid variable name
status 1
bash: line 5: x-y: invalid variable name
status 1
bash: line 6: : invalid variable name
status 1
bash: line 7: u: invalid indirect expansion
status 1
bash: line 8: x:-y: invalid variable name
status 1
end

== status 0
