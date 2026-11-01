== stdout 86
status 2
status 1
status 1
status 1 good=1 also=2
status 2
status 2
status 2
status 1

== stderr 715
bash: line 1: declare: -Z: invalid option
declare: usage: declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]
bash: line 3: declare: `1abc=3': not a valid identifier
bash: line 5: declare: `a b': not a valid identifier
bash: line 7: declare: `=bad': not a valid identifier
bash: line 9: typeset: -q: invalid option
typeset: usage: typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]
bash: line 11: readonly: -z: invalid option
readonly: usage: readonly [-aAf] [name[=value] ...] or readonly -p
bash: line 13: export: -q: invalid option
export: usage: export [-fn] [name[=value] ...] or export -p [-f]
bash: line 15: declare: `-': not a valid identifier

== merged 801
bash: line 1: declare: -Z: invalid option
declare: usage: declare [-aAfFgiIlnrtux] [name[=value] ...] or declare -p [-aAfFilnrtux] [name ...]
status 2
bash: line 3: declare: `1abc=3': not a valid identifier
status 1
bash: line 5: declare: `a b': not a valid identifier
status 1
bash: line 7: declare: `=bad': not a valid identifier
status 1 good=1 also=2
bash: line 9: typeset: -q: invalid option
typeset: usage: typeset [-aAfFgiIlnrtux] name[=value] ... or typeset -p [-aAfFilnrtux] [name ...]
status 2
bash: line 11: readonly: -z: invalid option
readonly: usage: readonly [-aAf] [name[=value] ...] or readonly -p
status 2
bash: line 13: export: -q: invalid option
export: usage: export [-fn] [name[=value] ...] or export -p [-f]
status 2
bash: line 15: declare: `-': not a valid identifier
status 1

== status 0
