== stdout 108
local st=1
declare st=1
cd st=1
cd2 st=2
dot st=1
source st=2
type st=1
unset st=0
eval st=2
eval2 st=2
end

== stderr 578
bash: line 1: local: can only be used in a function
bash: line 3: declare: nosuchvar: not found
bash: line 5: cd: /nonexistent: No such file or directory
bash: line 7: cd: too many arguments
bash: line 9: /nonexistent/file: No such file or directory
bash: line 11: source: filename argument required
source: usage: source [-p path] filename [arguments]
bash: line 13: type: nosuchcmd: not found
bash: eval: line 18: syntax error: unexpected end of file from `if' command on line 17
bash: eval: line 19: syntax error near unexpected token `newline'
bash: eval: line 19: `echo ('

== merged 686
bash: line 1: local: can only be used in a function
local st=1
bash: line 3: declare: nosuchvar: not found
declare st=1
bash: line 5: cd: /nonexistent: No such file or directory
cd st=1
bash: line 7: cd: too many arguments
cd2 st=2
bash: line 9: /nonexistent/file: No such file or directory
dot st=1
bash: line 11: source: filename argument required
source: usage: source [-p path] filename [arguments]
source st=2
bash: line 13: type: nosuchcmd: not found
type st=1
unset st=0
bash: eval: line 18: syntax error: unexpected end of file from `if' command on line 17
eval st=2
bash: eval: line 19: syntax error near unexpected token `newline'
bash: eval: line 19: `echo ('
eval2 st=2
end

== status 0
