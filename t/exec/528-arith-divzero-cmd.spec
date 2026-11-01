== stdout 92
div status 1
mod status 1
div-assign status 1 x=3
partial status 1 y=2
else 1
or-branch
end

== stderr 376
bash: line 1: ((: 1 / 0 : division by 0 (error token is "0 ")
bash: line 2: ((: 5 % 0 : division by 0 (error token is "0 ")
bash: line 4: ((: x /= 0 : division by 0 (error token is "0 ")
bash: line 5: ((: y = 2, 1 / 0 : division by 0 (error token is "0 ")
bash: line 6: ((: 1/0 : division by 0 (error token is "0 ")
bash: line 7: ((: 1/0 : division by 0 (error token is "0 ")

== merged 468
bash: line 1: ((: 1 / 0 : division by 0 (error token is "0 ")
div status 1
bash: line 2: ((: 5 % 0 : division by 0 (error token is "0 ")
mod status 1
bash: line 4: ((: x /= 0 : division by 0 (error token is "0 ")
div-assign status 1 x=3
bash: line 5: ((: y = 2, 1 / 0 : division by 0 (error token is "0 ")
partial status 1 y=2
bash: line 6: ((: 1/0 : division by 0 (error token is "0 ")
else 1
bash: line 7: ((: 1/0 : division by 0 (error token is "0 ")
or-branch
end

== status 0
