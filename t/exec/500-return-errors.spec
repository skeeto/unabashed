== stdout 79
a st=2
b st=2
c st=2
e st=44
f st=254
g st=1
sub st=4
h st=2
i st=6
j st=7
end

== stderr 276
bash: line 1: return: can only `return' from a function or sourced script
bash: line 3: return: can only `return' from a function or sourced script
bash: line 5: return: abc: numeric argument required
bash: line 19: return: can only `return' from a function or sourced script

== merged 355
bash: line 1: return: can only `return' from a function or sourced script
a st=2
bash: line 3: return: can only `return' from a function or sourced script
b st=2
bash: line 5: return: abc: numeric argument required
c st=2
e st=44
f st=254
g st=1
sub st=4
bash: line 19: return: can only `return' from a function or sourced script
h st=2
i st=6
j st=7
end

== status 0
