== stdout 42
1: 2
trap 2
2: 2
3: 0
4: 2
6: 2
7: 2
8: 2

== stderr 656
bash: eval: line 5: syntax error near unexpected token `)'
bash: eval: line 6: unexpected token `newline', conditional binary operator expected
bash: eval: line 7: syntax error near unexpected token `)'
bash: eval: line 7: `)'
bash: eval: line 7: syntax error near unexpected token `)'
bash: eval: line 7: `)'
bash: eval: line 8: syntax error near unexpected token `)'
bash: eval: line 8: `)'
bash: eval: line 9: syntax error near unexpected token `)'
/dev/stdin: line 2: syntax error: unexpected end of file from `(' command on line 1
bash: -c: line 1: syntax error near unexpected token `)'
bash: -c: line 1: syntax error near unexpected token `newline'

== merged 698
bash: eval: line 5: syntax error near unexpected token `)'
1: 2
bash: eval: line 6: unexpected token `newline', conditional binary operator expected
trap 2
2: 2
bash: eval: line 7: syntax error near unexpected token `)'
bash: eval: line 7: `)'
bash: eval: line 7: syntax error near unexpected token `)'
bash: eval: line 7: `)'
3: 0
bash: eval: line 8: syntax error near unexpected token `)'
bash: eval: line 8: `)'
4: 2
bash: eval: line 9: syntax error near unexpected token `)'
/dev/stdin: line 2: syntax error: unexpected end of file from `(' command on line 1
6: 2
bash: -c: line 1: syntax error near unexpected token `)'
7: 2
bash: -c: line 1: syntax error near unexpected token `newline'
8: 2

== status 0
