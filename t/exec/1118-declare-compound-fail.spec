== stdout 81
next 1
subshell 1
<>
eval 1
f 1
after f 0
next 1
h 1
after h 0
r 1
after r 0
end

== stderr 1081
bash: line 8: ro: readonly variable
bash: line 10: ro: readonly variable
bash: line 11: ro: readonly variable
bash: line 12: ro: readonly variable
bash: line 13: ro: readonly variable
bash: line 14: ro: readonly variable
bash: line 15: ro: readonly variable
bash: line 16: ro: readonly variable
bash: line 17: ro: readonly variable
bash: line 18: ro: readonly variable
bash: line 19: ro: readonly variable
bash: line 20: ro: readonly variable
bash: line 21: f: ro: readonly variable
bash: line 21: declare: ro: readonly variable
bash: line 22: ro: readonly variable
bash: line 24: ro: readonly variable
bash: line 25: h: ro: readonly variable
bash: line 25: declare: ro: readonly variable
bash: line 26: ro: readonly variable
bash: line 27: m: ro: readonly variable
bash: line 28: lr: readonly variable
bash: line 29: lr: readonly variable
bash: line 31: q: as: cannot convert associative to indexed array
bash: line 32: r: as: cannot convert associative to indexed array
bash: line 32: declare: as: cannot convert associative to indexed array
bash: line 33: ro: readonly variable

== merged 1162
bash: line 8: ro: readonly variable
next 1
bash: line 10: ro: readonly variable
bash: line 11: ro: readonly variable
bash: line 12: ro: readonly variable
bash: line 13: ro: readonly variable
bash: line 14: ro: readonly variable
bash: line 15: ro: readonly variable
bash: line 16: ro: readonly variable
bash: line 17: ro: readonly variable
bash: line 18: ro: readonly variable
subshell 1
bash: line 19: ro: readonly variable
<>
bash: line 20: ro: readonly variable
eval 1
bash: line 21: f: ro: readonly variable
bash: line 21: declare: ro: readonly variable
f 1
after f 0
bash: line 22: ro: readonly variable
next 1
bash: line 24: ro: readonly variable
bash: line 25: h: ro: readonly variable
bash: line 25: declare: ro: readonly variable
h 1
after h 0
bash: line 26: ro: readonly variable
bash: line 27: m: ro: readonly variable
bash: line 28: lr: readonly variable
bash: line 29: lr: readonly variable
bash: line 31: q: as: cannot convert associative to indexed array
bash: line 32: r: as: cannot convert associative to indexed array
bash: line 32: declare: as: cannot convert associative to indexed array
r 1
after r 0
bash: line 33: ro: readonly variable
end

== status 0
