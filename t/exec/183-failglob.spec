== stdout 68
have
status 1
assign *.none
after for 1
*.none
f status 1
null2
end

== stderr 158
bash: line 7: no match: *.none
bash: line 8: no match: *.none
bash: line 12: no match: *.none
bash: line 16: no match: *.none
bash: line 19: no match: *.none

== merged 226
have
bash: line 7: no match: *.none
bash: line 8: no match: *.none
status 1
assign *.none
bash: line 12: no match: *.none
after for 1
*.none
bash: line 16: no match: *.none
f status 1
bash: line 19: no match: *.none
null2
end

== status 0
