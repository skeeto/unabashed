== stdout 74
status 1
status 1
bad status 1
mixed status 1 good=1 ok=3
option status 2

== stderr 298
bash: line 1: local: can only be used in a function
bash: line 3: local: can only be used in a function
bash: line 6: local: `1bad=2': not a valid identifier
bash: line 8: local: `b c=2': not a valid identifier
bash: line 10: local: -Z: invalid option
local: usage: local [option] name[=value] ...

== merged 372
bash: line 1: local: can only be used in a function
status 1
bash: line 3: local: can only be used in a function
status 1
bash: line 6: local: `1bad=2': not a valid identifier
bad status 1
bash: line 8: local: `b c=2': not a valid identifier
mixed status 1 good=1 ok=3
bash: line 10: local: -Z: invalid option
local: usage: local [option] name[=value] ...
option status 2

== status 0
