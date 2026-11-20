== stdout 105
export -p 0
export -p assigns 0 [2]
2
declare -r Z="1"
declare -fx f
-pF 1
unset -n 1
unset -n plain [1]

== stderr 753
bash: line 5: declare: 1/0: division by 0 (error token is "0")
bash: line 6: local: 1/0: division by 0 (error token is "0")
bash: line 7: export: 1/0: division by 0 (error token is "0")
bash: line 8: read: 1/0: division by 0 (error token is "0")
bash: line 9: printf: 1/0: division by 0 (error token is "0")
bash: line 10: 1/0: division by 0 (error token is "0")
bash: line 11: declare: 1/0: division by 0 (error token is "0")
bash: line 12: 1/0: division by 0 (error token is "0")
bash: line 13: 1/0: division by 0 (error token is "0")
bash: line 14: declare: 1/0: division by 0 (error token is "0")
bash: line 15: 1/0: division by 0 (error token is "0")
bash: line 20: declare: nope: not found
bash: line 21: unset: R: cannot unset: readonly variable

== merged 858
bash: line 5: declare: 1/0: division by 0 (error token is "0")
bash: line 6: local: 1/0: division by 0 (error token is "0")
bash: line 7: export: 1/0: division by 0 (error token is "0")
bash: line 8: read: 1/0: division by 0 (error token is "0")
bash: line 9: printf: 1/0: division by 0 (error token is "0")
bash: line 10: 1/0: division by 0 (error token is "0")
bash: line 11: declare: 1/0: division by 0 (error token is "0")
bash: line 12: 1/0: division by 0 (error token is "0")
bash: line 13: 1/0: division by 0 (error token is "0")
bash: line 14: declare: 1/0: division by 0 (error token is "0")
bash: line 15: 1/0: division by 0 (error token is "0")
export -p 0
export -p assigns 0 [2]
2
declare -r Z="1"
declare -fx f
bash: line 20: declare: nope: not found
-pF 1
bash: line 21: unset: R: cannot unset: readonly variable
unset -n 1
unset -n plain [1]

== status 0
