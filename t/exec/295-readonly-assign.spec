== stdout 138
r=1
after assign status 1 r=1
declare status 1 r=1
readonly again status 1 r=1
append status 1 r=1
local status 1
r=1
export status 1 r=1

== stderr 228
bash: line 3: r: readonly variable
bash: line 5: declare: r: readonly variable
bash: line 7: r: readonly variable
bash: line 9: r: readonly variable
bash: line 11: local: r: readonly variable
bash: line 14: r: readonly variable

== merged 366
r=1
bash: line 3: r: readonly variable
after assign status 1 r=1
bash: line 5: declare: r: readonly variable
declare status 1 r=1
bash: line 7: r: readonly variable
readonly again status 1 r=1
bash: line 9: r: readonly variable
append status 1 r=1
bash: line 11: local: r: readonly variable
local status 1
r=1
bash: line 14: r: readonly variable
export status 1 r=1

== status 0
