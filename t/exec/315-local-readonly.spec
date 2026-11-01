== stdout 63
lr=locked
outside: [unset]
global lr=free
local G status 1 G=1

== stderr 116
bash: line 4: lr: readonly variable
bash: line 14: local: G: readonly variable
bash: line 17: G2: readonly variable

== merged 179
lr=locked
bash: line 4: lr: readonly variable
outside: [unset]
global lr=free
bash: line 14: local: G: readonly variable
local G status 1 G=1
bash: line 17: G2: readonly variable

== status 1
