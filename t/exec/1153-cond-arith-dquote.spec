== stdout 1156
-- [[ 3 -ge "\"" ]]; echo $?
0
-- [[ "\"3" -eq 3 ]]; echo $?
0
-- [[ 3 -eq "\"3\"" ]]; echo $?
0
-- [[ "1\"+2" -eq 3 ]]; echo $?
0
-- ab=4; [[ "a\"b" -eq 4 ]]; echo $?
0
-- [[ 3 -eq "\\\"3" ]]; echo $?
bash: line 1: [[: \3: arithmetic syntax error: operand expected (error token is "\3")
1
-- x=1; [[ -v "\"x" ]]; echo $?
0
-- set -x; [[ "\"3" -lt 4 ]]; echo $?
+ [[ 3 -lt 4 ]]
+ echo 0
0
-- [[ 3 -ge '"' ]]; echo $?
bash: line 1: [[: ": arithmetic syntax error: operand expected (error token is """)
1
-- x='"'; [[ 3 -ge "$x" ]]; echo $?
bash: line 1: [[: ": arithmetic syntax error: operand expected (error token is """)
1
-- [[ \"3 -eq 3 ]]; echo $?
bash: line 1: [[: "3: arithmetic syntax error: operand expected (error token is ""3")
1
-- [[ 3 -eq "${u:-\"3}" ]]; echo $?
bash: line 1: [[: "3: arithmetic syntax error: operand expected (error token is ""3")
1
-- [[ 3 -eq "$(echo \"3)" ]]; echo $?
bash: line 1: [[: "3: arithmetic syntax error: operand expected (error token is ""3")
1
-- [[ "\"" == '"' ]]; echo $?
0
-- [[ -n "\"" ]]; echo $?
0
-- echo $(( "\"3" ))
bash: line 1: "3 : arithmetic syntax error: operand expected (error token is ""3 ")

== stderr 0

== merged 1156
-- [[ 3 -ge "\"" ]]; echo $?
0
-- [[ "\"3" -eq 3 ]]; echo $?
0
-- [[ 3 -eq "\"3\"" ]]; echo $?
0
-- [[ "1\"+2" -eq 3 ]]; echo $?
0
-- ab=4; [[ "a\"b" -eq 4 ]]; echo $?
0
-- [[ 3 -eq "\\\"3" ]]; echo $?
bash: line 1: [[: \3: arithmetic syntax error: operand expected (error token is "\3")
1
-- x=1; [[ -v "\"x" ]]; echo $?
0
-- set -x; [[ "\"3" -lt 4 ]]; echo $?
+ [[ 3 -lt 4 ]]
+ echo 0
0
-- [[ 3 -ge '"' ]]; echo $?
bash: line 1: [[: ": arithmetic syntax error: operand expected (error token is """)
1
-- x='"'; [[ 3 -ge "$x" ]]; echo $?
bash: line 1: [[: ": arithmetic syntax error: operand expected (error token is """)
1
-- [[ \"3 -eq 3 ]]; echo $?
bash: line 1: [[: "3: arithmetic syntax error: operand expected (error token is ""3")
1
-- [[ 3 -eq "${u:-\"3}" ]]; echo $?
bash: line 1: [[: "3: arithmetic syntax error: operand expected (error token is ""3")
1
-- [[ 3 -eq "$(echo \"3)" ]]; echo $?
bash: line 1: [[: "3: arithmetic syntax error: operand expected (error token is ""3")
1
-- [[ "\"" == '"' ]]; echo $?
0
-- [[ -n "\"" ]]; echo $?
0
-- echo $(( "\"3" ))
bash: line 1: "3 : arithmetic syntax error: operand expected (error token is ""3 ")

== status 1
