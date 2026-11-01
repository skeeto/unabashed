== stdout 100
before
after 1
next-line 1
e3
after-eval-same-line 0
subshell status 1
cmdsub [] 1
after-loop 1
end

== stderr 399
bash: line 2: 1 / 0 : division by 0 (error token is "0 ")
bash: line 5: 1/0 : division by 0 (error token is "0 ")
bash: line 10: 1/0 : division by 0 (error token is "0 ")
bash: line 12: 1/0 : division by 0 (error token is "0 ")
bash: line 13: 1/0 : division by 0 (error token is "0 ")
bash: line 15: i/0 : division by 0 (error token is "0 ")
bash: line 18: 1/0 : division by 0 (error token is "0 ")

== merged 499
before
bash: line 2: 1 / 0 : division by 0 (error token is "0 ")
after 1
bash: line 5: 1/0 : division by 0 (error token is "0 ")
next-line 1
bash: line 10: 1/0 : division by 0 (error token is "0 ")
e3
after-eval-same-line 0
bash: line 12: 1/0 : division by 0 (error token is "0 ")
subshell status 1
bash: line 13: 1/0 : division by 0 (error token is "0 ")
cmdsub [] 1
bash: line 15: i/0 : division by 0 (error token is "0 ")
after-loop 1
bash: line 18: 1/0 : division by 0 (error token is "0 ")
end

== status 0
