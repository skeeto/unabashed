== stdout 57
$?=1
st=127
$?=1
h 0
$?=1
passed on
1
() { if; }
quiet 0

== stderr 1298
bash: f: line 0: syntax error near unexpected token `('
bash: f: line 0: `f () {  [ *([0-9]) = x ];'
bash: error importing function definition for `f'
bash: line 1: f: command not found
bash: warning: g: ignoring function definition attempt
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: g: line 1: syntax error: unexpected end of file from `{' command on line 0
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: g: line 1: syntax error near unexpected token `;'
bash: g: line 1: `  ( ; }'
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: g: line 0: unexpected EOF while looking for matching `)'
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: g: line 1: syntax error near unexpected token `}'
bash: g: line 1: `}'
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: error importing function definition for `a/b'
bash: g: line 0: syntax error near unexpected token `;'
bash: g: line 0: `g () { if; }'
bash: error importing function definition for `g'
bash: g: line 0: syntax error near unexpected token `;'
bash: g: line 0: `g () { if; }'
bash: error importing function definition for `g'

== merged 1355
bash: f: line 0: syntax error near unexpected token `('
bash: f: line 0: `f () {  [ *([0-9]) = x ];'
bash: error importing function definition for `f'
$?=1
bash: line 1: f: command not found
st=127
bash: warning: g: ignoring function definition attempt
bash: error importing function definition for `g'
$?=1
bash: line 1: g: command not found
bash: g: line 1: syntax error: unexpected end of file from `{' command on line 0
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: g: line 1: syntax error near unexpected token `;'
bash: g: line 1: `  ( ; }'
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: g: line 0: unexpected EOF while looking for matching `)'
bash: error importing function definition for `g'
bash: line 1: g: command not found
bash: g: line 1: syntax error near unexpected token `}'
bash: g: line 1: `}'
bash: error importing function definition for `g'
h 0
bash: line 1: g: command not found
bash: error importing function definition for `a/b'
$?=1
passed on
bash: g: line 0: syntax error near unexpected token `;'
bash: g: line 0: `g () { if; }'
bash: error importing function definition for `g'
bash: g: line 0: syntax error near unexpected token `;'
bash: g: line 0: `g () { if; }'
bash: error importing function definition for `g'
1
() { if; }
quiet 0

== status 0
