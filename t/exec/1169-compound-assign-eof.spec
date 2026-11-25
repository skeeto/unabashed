== stdout 48
1: 1
2: 2
3: 2
4: 1 1
4: 2 2
5: 
6: 1
7: 1
8: 1

== stderr 754
bash: eval: line 4: unexpected EOF while looking for matching `)'
bash: eval: line 5: syntax error near unexpected token `newline'
bash: eval: line 5: `x=('
bash: eval: line 6: unexpected EOF while looking for matching `)'
bash: eval: line 6: syntax error near unexpected token `newline'
bash: eval: line 6: `x=('
bash: eval: line 7: unexpected EOF while looking for matching `)'
bash: eval: line 7: syntax error near unexpected token `newline'
bash: eval: line 7: `x=('
bash: eval: line 8: unexpected EOF while looking for matching `)'
bash: eval: line 10: unexpected EOF while looking for matching `)'
bash: eval: line 11: unexpected EOF while looking for matching `)'
bash: command substitution: line 17: unexpected EOF while looking for matching `''

== merged 802
bash: eval: line 4: unexpected EOF while looking for matching `)'
1: 1
bash: eval: line 5: syntax error near unexpected token `newline'
bash: eval: line 5: `x=('
2: 2
bash: eval: line 6: unexpected EOF while looking for matching `)'
bash: eval: line 6: syntax error near unexpected token `newline'
bash: eval: line 6: `x=('
3: 2
bash: eval: line 7: unexpected EOF while looking for matching `)'
4: 1 1
bash: eval: line 7: syntax error near unexpected token `newline'
bash: eval: line 7: `x=('
4: 2 2
bash: eval: line 8: unexpected EOF while looking for matching `)'
5: 
bash: eval: line 10: unexpected EOF while looking for matching `)'
6: 1
bash: eval: line 11: unexpected EOF while looking for matching `)'
7: 1
bash: command substitution: line 17: unexpected EOF while looking for matching `''
8: 1

== status 0
