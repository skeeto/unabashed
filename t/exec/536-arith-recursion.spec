== stdout 42
chain 7
self status 1
mutual status 1
end

== stderr 156
bash: line 6: ((: r: expression recursion level exceeded (error token is "r")
bash: line 8: ((: q: expression recursion level exceeded (error token is "q")

== merged 198
chain 7
bash: line 6: ((: r: expression recursion level exceeded (error token is "r")
self status 1
bash: line 8: ((: q: expression recursion level exceeded (error token is "q")
mutual status 1
end

== status 0
