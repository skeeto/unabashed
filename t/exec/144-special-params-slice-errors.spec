== stdout 31
status 1
status 1
status 1
end

== stderr 143
bash: line 3: -1: substring expression < 0
bash: line 4: -2: substring expression < 0
bash: line 5: @: 1/0: division by 0 (error token is "0")

== merged 174
bash: line 3: -1: substring expression < 0
status 1
bash: line 4: -2: substring expression < 0
status 1
bash: line 5: @: 1/0: division by 0 (error token is "0")
status 1
end

== status 0
