== stdout 312
readonly 4
export 4
declare 4
typeset 4
nameref 4
self 4
empty 4
export sub 4
readonly word 1
mixed 4
attribute 1
attribute nameref 1
command 4
unset 7
unset name 7
unset stack 7
unset -n 7
stage 0 7 0
async 4
function 4
shadow 1
local 4
group 1
eval 1
comsub 1
subshell 1
coproc 1
plain 1
plain unset 1
posix 7

== stderr 1453
bash: line 7: R: readonly variable
bash: line 8: R: readonly variable
bash: line 9: declare: R: readonly variable
bash: line 10: typeset: `1bad': not a valid identifier
bash: line 11: declare: `1bad': invalid variable name for name reference
bash: line 12: declare: N: nameref variable self references not allowed
bash: line 13: declare: `': not a valid identifier
bash: line 14: export: `A[1]': not a valid identifier
bash: line 15: readonly: `1bad=1': not a valid identifier
bash: line 16: R: readonly variable
bash: line 16: readonly: `1bad': not a valid identifier
bash: line 17: declare: R: readonly variable
bash: line 18: declare: R: readonly variable
bash: line 19: R: readonly variable
bash: line 20: unset: R: cannot unset: readonly variable
bash: line 21: unset: `1bad': not a valid identifier
bash: line 22: unset: BASH_ARGV: cannot unset
bash: line 23: unset: R: cannot unset: readonly variable
bash: line 24: unset: R: cannot unset: readonly variable
bash: line 25: R: readonly variable
bash: line 26: R: readonly variable
bash: line 27: local: R: readonly variable
bash: line 28: local: L: readonly variable
bash: line 29: R: readonly variable
bash: line 30: R: readonly variable
bash: line 31: R: readonly variable
bash: line 32: R: readonly variable
bash: line 33: R: readonly variable
bash: line 34: R: readonly variable
bash: line 35: unset: R: cannot unset: readonly variable
bash: line 36: unset: R: cannot unset: readonly variable

== merged 1765
bash: line 7: R: readonly variable
readonly 4
bash: line 8: R: readonly variable
export 4
bash: line 9: declare: R: readonly variable
declare 4
bash: line 10: typeset: `1bad': not a valid identifier
typeset 4
bash: line 11: declare: `1bad': invalid variable name for name reference
nameref 4
bash: line 12: declare: N: nameref variable self references not allowed
self 4
bash: line 13: declare: `': not a valid identifier
empty 4
bash: line 14: export: `A[1]': not a valid identifier
export sub 4
bash: line 15: readonly: `1bad=1': not a valid identifier
readonly word 1
bash: line 16: R: readonly variable
bash: line 16: readonly: `1bad': not a valid identifier
mixed 4
bash: line 17: declare: R: readonly variable
attribute 1
bash: line 18: declare: R: readonly variable
attribute nameref 1
bash: line 19: R: readonly variable
command 4
bash: line 20: unset: R: cannot unset: readonly variable
unset 7
bash: line 21: unset: `1bad': not a valid identifier
unset name 7
bash: line 22: unset: BASH_ARGV: cannot unset
unset stack 7
bash: line 23: unset: R: cannot unset: readonly variable
unset -n 7
bash: line 24: unset: R: cannot unset: readonly variable
stage 0 7 0
bash: line 25: R: readonly variable
async 4
bash: line 26: R: readonly variable
function 4
bash: line 27: local: R: readonly variable
shadow 1
bash: line 28: local: L: readonly variable
local 4
bash: line 29: R: readonly variable
group 1
bash: line 30: R: readonly variable
eval 1
bash: line 31: R: readonly variable
comsub 1
bash: line 32: R: readonly variable
subshell 1
bash: line 33: R: readonly variable
coproc 1
bash: line 34: R: readonly variable
plain 1
bash: line 35: unset: R: cannot unset: readonly variable
plain unset 1
bash: line 36: unset: R: cannot unset: readonly variable
posix 7

== status 0
