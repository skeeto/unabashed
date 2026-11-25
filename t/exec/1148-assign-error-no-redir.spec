== stdout 85
status 1 fd=unset
status 1 a=1 c=unset
status 1 a2=unset
status 1
status 0
files: d7

== stderr 313
bash: line 6: arr[]: bad array subscript
bash: line 8: arr[]: bad array subscript
bash: line 9: b[]: bad array subscript
bash: line 12: r: readonly variable
bash: line 13: r: readonly variable
sub
bash: line 15: e[]: bad array subscript
bash: line 17: r: readonly variable
bash: line 21: e[]: bad array subscript

== merged 398
bash: line 6: arr[]: bad array subscript
status 1 fd=unset
bash: line 8: arr[]: bad array subscript
bash: line 9: b[]: bad array subscript
status 1 a=1 c=unset
bash: line 12: r: readonly variable
bash: line 13: r: readonly variable
status 1 a2=unset
sub
bash: line 15: e[]: bad array subscript
status 1
bash: line 17: r: readonly variable
status 0
files: d7
bash: line 21: e[]: bad array subscript

== status 127
