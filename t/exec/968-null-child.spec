== stdout 161
dup 0
file 0
herestring 0
close 0
output 0
assign 0 []
other fd 1
fd 3 1
failed 1
var 0 []
alone []
fd 10 1
readonly 1
function 0
survived
errexit 1
ERR
async 0

== stderr 179
bash: line 12: /nonexistent: No such file or directory
bash: line 15: 10: Bad file descriptor
bash: line 16: fd: readonly variable
bash: line 16: fd: cannot assign fd to variable

== merged 340
dup 0
file 0
herestring 0
close 0
output 0
assign 0 []
other fd 1
fd 3 1
bash: line 12: /nonexistent: No such file or directory
failed 1
var 0 []
alone []
bash: line 15: 10: Bad file descriptor
fd 10 1
bash: line 16: fd: readonly variable
bash: line 16: fd: cannot assign fd to variable
readonly 1
function 0
survived
errexit 1
ERR
async 0

== status 0
