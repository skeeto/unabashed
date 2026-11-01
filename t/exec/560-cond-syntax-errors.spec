== stdout 261
2 <- [[ a -eq ]]
2 <- [[ ( a ]]
2 <- [[ a b ]]
2 <- [[ ]]
2 <- [[ a == ]]
2 <- [[ && a ]]
2 <- [[ a && ]]
2 <- [[ -n a b ]]
2 <- [[ a -foo b ]]
2 <- [[ ( ) ]]
2 <- [[ a ) ]]
2 <- [[ a "==" a ]]
2 <- [[ a \== a ]]
2 <- [[ "-n" a ]]
end
runtime 2
two
multiline 2

== stderr 2209
bash: eval: line 1: unexpected argument `]]' to conditional binary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ a -eq ]]'
bash: eval: line 1: unexpected token `]]', expected `)'
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ ( a ]]'
bash: eval: line 1: unexpected token `b', conditional binary operator expected
bash: eval: line 1: syntax error near `b'
bash: eval: line 1: `[[ a b ]]'
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ ]]'
bash: eval: line 1: unexpected argument `]]' to conditional binary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ a == ]]'
bash: eval: line 1: unexpected token `&&' in conditional command
bash: eval: line 1: syntax error near `&'
bash: eval: line 1: `[[ && a ]]'
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ a && ]]'
bash: eval: line 1: syntax error in conditional expression: unexpected token `b'
bash: eval: line 1: syntax error near `b'
bash: eval: line 1: `[[ -n a b ]]'
bash: eval: line 1: unexpected token `-foo', conditional binary operator expected
bash: eval: line 1: syntax error near `-foo'
bash: eval: line 1: `[[ a -foo b ]]'
bash: eval: line 1: unexpected token `)' in conditional command
bash: eval: line 1: expected `)'
bash: eval: line 1: syntax error near `)'
bash: eval: line 1: `[[ ( ) ]]'
bash: eval: line 1: syntax error in conditional expression: unexpected token `)'
bash: eval: line 1: syntax error near `)'
bash: eval: line 1: `[[ a ) ]]'
bash: eval: line 1: unexpected token `"=="', conditional binary operator expected
bash: eval: line 1: syntax error near `"=="'
bash: eval: line 1: `[[ a "==" a ]]'
bash: eval: line 1: unexpected token `\==', conditional binary operator expected
bash: eval: line 1: syntax error near `\=='
bash: eval: line 1: `[[ a \== a ]]'
bash: eval: line 1: unexpected token `a', conditional binary operator expected
bash: eval: line 1: syntax error near `a'
bash: eval: line 1: `[[ "-n" a ]]'
bash: line 17: test: x: unary operator expected
bash: eval: line 20: unexpected token `b', conditional binary operator expected
bash: eval: line 20: syntax error near `b'
bash: eval: line 20: `[[ a b ]]'

== merged 2470
bash: eval: line 1: unexpected argument `]]' to conditional binary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ a -eq ]]'
2 <- [[ a -eq ]]
bash: eval: line 1: unexpected token `]]', expected `)'
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ ( a ]]'
2 <- [[ ( a ]]
bash: eval: line 1: unexpected token `b', conditional binary operator expected
bash: eval: line 1: syntax error near `b'
bash: eval: line 1: `[[ a b ]]'
2 <- [[ a b ]]
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ ]]'
2 <- [[ ]]
bash: eval: line 1: unexpected argument `]]' to conditional binary operator
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ a == ]]'
2 <- [[ a == ]]
bash: eval: line 1: unexpected token `&&' in conditional command
bash: eval: line 1: syntax error near `&'
bash: eval: line 1: `[[ && a ]]'
2 <- [[ && a ]]
bash: eval: line 1: syntax error near `]]'
bash: eval: line 1: `[[ a && ]]'
2 <- [[ a && ]]
bash: eval: line 1: syntax error in conditional expression: unexpected token `b'
bash: eval: line 1: syntax error near `b'
bash: eval: line 1: `[[ -n a b ]]'
2 <- [[ -n a b ]]
bash: eval: line 1: unexpected token `-foo', conditional binary operator expected
bash: eval: line 1: syntax error near `-foo'
bash: eval: line 1: `[[ a -foo b ]]'
2 <- [[ a -foo b ]]
bash: eval: line 1: unexpected token `)' in conditional command
bash: eval: line 1: expected `)'
bash: eval: line 1: syntax error near `)'
bash: eval: line 1: `[[ ( ) ]]'
2 <- [[ ( ) ]]
bash: eval: line 1: syntax error in conditional expression: unexpected token `)'
bash: eval: line 1: syntax error near `)'
bash: eval: line 1: `[[ a ) ]]'
2 <- [[ a ) ]]
bash: eval: line 1: unexpected token `"=="', conditional binary operator expected
bash: eval: line 1: syntax error near `"=="'
bash: eval: line 1: `[[ a "==" a ]]'
2 <- [[ a "==" a ]]
bash: eval: line 1: unexpected token `\==', conditional binary operator expected
bash: eval: line 1: syntax error near `\=='
bash: eval: line 1: `[[ a \== a ]]'
2 <- [[ a \== a ]]
bash: eval: line 1: unexpected token `a', conditional binary operator expected
bash: eval: line 1: syntax error near `a'
bash: eval: line 1: `[[ "-n" a ]]'
2 <- [[ "-n" a ]]
end
bash: line 17: test: x: unary operator expected
runtime 2
two
bash: eval: line 20: unexpected token `b', conditional binary operator expected
bash: eval: line 20: syntax error near `b'
bash: eval: line 20: `[[ a b ]]'
multiline 2

== status 0
