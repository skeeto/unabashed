== stdout 91
assign status 1 ro=5
inc status 1 ro=5
let status 1 ro=5
other=6
end
after-exp 1 ro=5
last

== stderr 181
bash: line 2: ro: readonly variable
bash: line 3: ro: readonly variable
bash: line 4: ro: readonly variable
bash: line 7: ro: readonly variable
bash: line 11: ri: readonly variable

== merged 272
bash: line 2: ro: readonly variable
assign status 1 ro=5
bash: line 3: ro: readonly variable
inc status 1 ro=5
bash: line 4: ro: readonly variable
let status 1 ro=5
other=6
end
bash: line 7: ro: readonly variable
after-exp 1 ro=5
bash: line 11: ri: readonly variable
last

== status 0
