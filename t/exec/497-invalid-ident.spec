== stdout 150
for st=1
unset st=0
unset-v st=1
export st=1
readonly st=1
declare st=1
read st=1
local st=1
assign-as-cmd st=127
unset-f st=0
multi st=1 a=1 c=3
end

== stderr 454
bash: line 1: `1x': not a valid identifier
bash: line 5: unset: `a b': not a valid identifier
bash: line 7: export: `1x=2': not a valid identifier
bash: line 9: readonly: `a b=1': not a valid identifier
bash: line 11: declare: `3=4': not a valid identifier
bash: line 13: read: `1x': not a valid identifier
bash: line 15: local: `9z=1': not a valid identifier
bash: line 17: x-y=1: command not found
bash: line 21: export: `2b=2': not a valid identifier

== merged 604
bash: line 1: `1x': not a valid identifier
for st=1
unset st=0
bash: line 5: unset: `a b': not a valid identifier
unset-v st=1
bash: line 7: export: `1x=2': not a valid identifier
export st=1
bash: line 9: readonly: `a b=1': not a valid identifier
readonly st=1
bash: line 11: declare: `3=4': not a valid identifier
declare st=1
bash: line 13: read: `1x': not a valid identifier
read st=1
bash: line 15: local: `9z=1': not a valid identifier
local st=1
bash: line 17: x-y=1: command not found
assign-as-cmd st=127
unset-f st=0
bash: line 21: export: `2b=2': not a valid identifier
multi st=1 a=1 c=3
end

== status 0
