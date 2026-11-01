== stdout 243
declare -A A=(["80's"]="Depeche Mode" ["]"]="rbracket" ["["]="lbracket" ["\$HOME"]="home" )
declare -A A=(["80's"]="Depeche Mode" ["\$HOME"]="home" )
declare -A myarray=([x]="y" ["foo[bar"]="bleh" )
declare -A a=(["\" \""]="11" )
5
notreached

== stderr 472
bash: line 12: read: `A[]]': not a valid identifier
bash: line 15: typeset: `foo[foo]bar]=bax': not a valid identifier
bash: line 19: $(echo INJECTION! >&2 ; echo 0): arithmetic syntax error: operand expected (error token is "$(echo INJECTION! >&2 ; echo 0)")
bash: line 21: $(echo INJECTION! >&2 ; echo 0): arithmetic syntax error: operand expected (error token is "$(echo INJECTION! >&2 ; echo 0)")
bash: line 22: foo[bar: bad array subscript (error token is "foo[bar")

== merged 715
declare -A A=(["80's"]="Depeche Mode" ["]"]="rbracket" ["["]="lbracket" ["\$HOME"]="home" )
declare -A A=(["80's"]="Depeche Mode" ["\$HOME"]="home" )
bash: line 12: read: `A[]]': not a valid identifier
declare -A myarray=([x]="y" ["foo[bar"]="bleh" )
bash: line 15: typeset: `foo[foo]bar]=bax': not a valid identifier
declare -A a=(["\" \""]="11" )
5
bash: line 19: $(echo INJECTION! >&2 ; echo 0): arithmetic syntax error: operand expected (error token is "$(echo INJECTION! >&2 ; echo 0)")
notreached
bash: line 21: $(echo INJECTION! >&2 ; echo 0): arithmetic syntax error: operand expected (error token is "$(echo INJECTION! >&2 ; echo 0)")
bash: line 22: foo[bar: bad array subscript (error token is "foo[bar")

== status 1
