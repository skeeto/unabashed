== stdout 1073
bash: line 6: typeset: q: readonly variable
s=1
declare -n q="z"
declare -r z="1"
declare -n r="w"
declare -r w
bash: line 8: typeset: r: readonly variable
s=1
declare -n r="w"
declare -r w
bash: line 9: typeset: w: readonly variable
s=1
declare -n r="w"
declare -r w
s=0
declare -nr ro="v"
declare -- v="x"
s=0
declare -nr ro="v"
declare -- v="x"
bash: line 12: typeset: w: readonly variable
s=1
bash: line 12: typeset: w: readonly variable
s=1
bash: line 12: typeset: r: readonly variable
s=1
bash: line 13: local: gl: readonly variable
s=1
bash: line 13: typeset: gl: readonly variable
s=1
bash: line 14: typeset: z: readonly variable
s=1
bash: line 14: typeset: z: readonly variable
s=1
bash: line 15: typeset: r: readonly variable
s=1
bash: line 15: typeset: q: readonly variable
s=1
declare -nr rr
bash: line 17: typeset: rr: readonly variable
s=1
declare -nr rr
s=0
declare -nr rr
bash: line 19: typeset: rr2: readonly variable
s=1
declare -nr rr2
bash: line 20: typeset: rr3: readonly variable
s=1
bash: line 20: typeset: rr3: readonly variable
s=1
declare -nr rr3

== stderr 0

== merged 1073
bash: line 6: typeset: q: readonly variable
s=1
declare -n q="z"
declare -r z="1"
declare -n r="w"
declare -r w
bash: line 8: typeset: r: readonly variable
s=1
declare -n r="w"
declare -r w
bash: line 9: typeset: w: readonly variable
s=1
declare -n r="w"
declare -r w
s=0
declare -nr ro="v"
declare -- v="x"
s=0
declare -nr ro="v"
declare -- v="x"
bash: line 12: typeset: w: readonly variable
s=1
bash: line 12: typeset: w: readonly variable
s=1
bash: line 12: typeset: r: readonly variable
s=1
bash: line 13: local: gl: readonly variable
s=1
bash: line 13: typeset: gl: readonly variable
s=1
bash: line 14: typeset: z: readonly variable
s=1
bash: line 14: typeset: z: readonly variable
s=1
bash: line 15: typeset: r: readonly variable
s=1
bash: line 15: typeset: q: readonly variable
s=1
declare -nr rr
bash: line 17: typeset: rr: readonly variable
s=1
declare -nr rr
s=0
declare -nr rr
bash: line 19: typeset: rr2: readonly variable
s=1
declare -nr rr2
bash: line 20: typeset: rr3: readonly variable
s=1
bash: line 20: typeset: rr3: readonly variable
s=1
declare -nr rr3

== status 0
