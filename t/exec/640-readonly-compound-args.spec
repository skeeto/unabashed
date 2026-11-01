== stdout 112
bash: line 19: declare: b: not found
declare -ar c=([0]="(3)")
declare -ax e=([0]="(3)")
declare -a d=([0]="3")

== stderr 712
bash: line 6: a: readonly variable
bash: line 7: a: readonly variable
bash: line 8: a: readonly variable
bash: line 9: a: readonly variable
bash: line 10: readonly: a: readonly variable
bash: line 11: declare: a: readonly variable
bash: line 12: a: readonly variable
bash: line 13: f: a: readonly variable
bash: line 13: declare: a: readonly variable
bash: line 14: a: readonly variable
bash: line 14: declare: a: readonly variable
bash: line 15: a: readonly variable
bash: line 16: f: a: readonly variable
bash: line 17: f: a: readonly variable
bash: line 18: f: a: readonly variable
bash: line 18: local: a: readonly variable
bash: line 19: f: a: readonly variable
bash: line 19: declare: a: readonly variable

== merged 824
bash: line 6: a: readonly variable
bash: line 7: a: readonly variable
bash: line 8: a: readonly variable
bash: line 9: a: readonly variable
bash: line 10: readonly: a: readonly variable
bash: line 11: declare: a: readonly variable
bash: line 12: a: readonly variable
bash: line 13: f: a: readonly variable
bash: line 13: declare: a: readonly variable
bash: line 14: a: readonly variable
bash: line 14: declare: a: readonly variable
bash: line 15: a: readonly variable
bash: line 16: f: a: readonly variable
bash: line 17: f: a: readonly variable
bash: line 18: f: a: readonly variable
bash: line 18: local: a: readonly variable
bash: line 19: f: a: readonly variable
bash: line 19: declare: a: readonly variable
bash: line 19: declare: b: not found
declare -ar c=([0]="(3)")
declare -ax e=([0]="(3)")
declare -a d=([0]="3")

== status 0
