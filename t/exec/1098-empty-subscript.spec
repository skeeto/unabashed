== stdout 410
declare -n 1
bash: line 7: declare: r: not found
declare -n assoc 1
declare -n self 1
local -n 1
declare -n e
append 1
declare -n ar="a"
append 1
append 0
declare -n ar="a[1]"
append new 1
for x
for 1
unset 0
unset assoc 0
unset -v 1
unset a[$k] 0
-v 1
test -v 1
read 1
read second 1
read -a 1
printf -v 2
wait -p 1
read once 1
printf -v once 2
declare 1
declare -a a=([0]="x" [1]="y")
declare -A h=([k]="v" )

== stderr 1240
bash: line 7: declare: `a[]': invalid variable name for name reference
bash: line 8: declare: `h[]': invalid variable name for name reference
bash: line 9: declare: `r[]': invalid variable name for name reference
bash: line 10: local: `a[]': invalid variable name for name reference
bash: line 12: `a[]': not a valid identifier
bash: line 14: declare: `[]': not a valid identifier
bash: line 15: declare: `[]x': not a valid identifier
bash: line 17: declare: `a[]': not a valid identifier
bash: line 18: `a[]': not a valid identifier
bash: line 19: `'a[]'': not a valid identifier
bash: line 22: unset: `a[]': not a valid identifier
bash: line 26: read: `a[]': not a valid identifier
bash: line 27: read: `h[]': not a valid identifier
bash: line 28: read: `a[]': not a valid identifier
bash: line 29: printf: `a[]': not a valid identifier
bash: line 30: wait: `a[]': not a valid identifier
bash: line 32: read: `h[]': not a valid identifier
bash: line 33: printf: `h[]': not a valid identifier
bash: line 35: a[]: invalid variable name
bash: line 36: ${a[]}: bad substitution
bash: line 37: ${#a[]}: bad substitution
bash: line 38: ${a[]:-d}: bad substitution
bash: line 39: a[]: bad array subscript
bash: line 40: a[]: bad array subscript

== merged 1650
bash: line 7: declare: `a[]': invalid variable name for name reference
declare -n 1
bash: line 7: declare: r: not found
bash: line 8: declare: `h[]': invalid variable name for name reference
declare -n assoc 1
bash: line 9: declare: `r[]': invalid variable name for name reference
declare -n self 1
bash: line 10: local: `a[]': invalid variable name for name reference
local -n 1
bash: line 12: `a[]': not a valid identifier
declare -n e
bash: line 14: declare: `[]': not a valid identifier
append 1
declare -n ar="a"
bash: line 15: declare: `[]x': not a valid identifier
append 1
append 0
declare -n ar="a[1]"
bash: line 17: declare: `a[]': not a valid identifier
append new 1
for x
bash: line 18: `a[]': not a valid identifier
for 1
bash: line 19: `'a[]'': not a valid identifier
unset 0
unset assoc 0
bash: line 22: unset: `a[]': not a valid identifier
unset -v 1
unset a[$k] 0
-v 1
test -v 1
bash: line 26: read: `a[]': not a valid identifier
read 1
bash: line 27: read: `h[]': not a valid identifier
read second 1
bash: line 28: read: `a[]': not a valid identifier
read -a 1
bash: line 29: printf: `a[]': not a valid identifier
printf -v 2
bash: line 30: wait: `a[]': not a valid identifier
wait -p 1
bash: line 32: read: `h[]': not a valid identifier
read once 1
bash: line 33: printf: `h[]': not a valid identifier
printf -v once 2
bash: line 35: a[]: invalid variable name
bash: line 36: ${a[]}: bad substitution
bash: line 37: ${#a[]}: bad substitution
bash: line 38: ${a[]:-d}: bad substitution
bash: line 39: a[]: bad array subscript
bash: line 40: a[]: bad array subscript
declare 1
declare -a a=([0]="x" [1]="y")
declare -A h=([k]="v" )

== status 0
