== stdout 49
first-line
status 2
ok
status 2
a
status 2
alive

== stderr 249
/tmp/bad.sh: line 2: syntax error near unexpected token `then'
/tmp/bad.sh: line 2: `if then'
/tmp/bad2.sh: line 2: unexpected EOF while looking for matching `"'
/tmp/bad3.sh: line 2: syntax error near unexpected token `)'
/tmp/bad3.sh: line 2: `)'

== merged 298
first-line
/tmp/bad.sh: line 2: syntax error near unexpected token `then'
/tmp/bad.sh: line 2: `if then'
status 2
ok
/tmp/bad2.sh: line 2: unexpected EOF while looking for matching `"'
status 2
a
/tmp/bad3.sh: line 2: syntax error near unexpected token `)'
/tmp/bad3.sh: line 2: `)'
status 2
alive

== status 0
