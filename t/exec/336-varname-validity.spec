== stdout 55
1 3
status 1
status 1
status 1
local status 1
status 1

== stderr 251
bash: line 3: declare: `x-y=1': not a valid identifier
bash: line 5: readonly: `r-r=1': not a valid identifier
bash: line 7: export: `e.e': not a valid identifier
bash: line 9: local: `%': not a valid identifier
bash: line 11: declare: p q: not found

== merged 306
1 3
bash: line 3: declare: `x-y=1': not a valid identifier
status 1
bash: line 5: readonly: `r-r=1': not a valid identifier
status 1
bash: line 7: export: `e.e': not a valid identifier
status 1
bash: line 9: local: `%': not a valid identifier
local status 1
bash: line 11: declare: p q: not found
status 1

== status 0
