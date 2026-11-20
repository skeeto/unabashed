== stdout 129
declare -a A=([0]="a" [1]="b c" [3]="d")
declare -a A=([0]="a" [1]="b")
st 1
declare -a C=([0]="([1)")
eval 1
subshell 1
posix 1

== stderr 977
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `"'
bash: array assign: line 9: syntax error near unexpected token `;'
bash: array assign: line 9: `a ;b'
bash: array assign: line 10: syntax error near unexpected token `|'
bash: array assign: line 10: `a |b'
bash: array assign: line 11: syntax error near unexpected token `('
bash: array assign: line 11: `a (b)'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'

== merged 1106
declare -a A=([0]="a" [1]="b c" [3]="d")
declare -a A=([0]="a" [1]="b")
bash: array assign: line 1: unexpected EOF while looking for matching `]'
st 1
bash: array assign: line 1: unexpected EOF while looking for matching `"'
bash: array assign: line 9: syntax error near unexpected token `;'
bash: array assign: line 9: `a ;b'
bash: array assign: line 10: syntax error near unexpected token `|'
bash: array assign: line 10: `a |b'
bash: array assign: line 11: syntax error near unexpected token `('
bash: array assign: line 11: `a (b)'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
declare -a C=([0]="([1)")
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
bash: array assign: line 1: unexpected EOF while looking for matching `]'
eval 1
bash: array assign: line 1: unexpected EOF while looking for matching `]'
subshell 1
bash: array assign: line 1: unexpected EOF while looking for matching `]'
posix 1

== status 0
