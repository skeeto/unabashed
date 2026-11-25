== stdout 1430
-- [[ -f a 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ ( a ) 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ ( a || b ) ]]'
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ a == b "c
bash: -c: line 1: unexpected EOF while looking for matching `"'
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ a && -n b 'c
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ a 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: conditional binary operator expected
status 2
-- [[ 'a
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: unexpected token `ÿ' in conditional command
status 2
-- [[ ( 'a
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: unexpected token `ÿ' in conditional command
bash: -c: line 1: expected `)'
status 2
-- [[ -f 'a
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: unexpected argument to conditional unary operator
status 2
-- [[ a ]] 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
status 2

== stderr 0

== merged 1430
-- [[ -f a 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ ( a ) 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ ( a || b ) ]]'
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ a == b "c
bash: -c: line 1: unexpected EOF while looking for matching `"'
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ a && -n b 'c
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 1: unexpected EOF while looking for `]]'
status 2
-- [[ a 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: conditional binary operator expected
status 2
-- [[ 'a
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: unexpected token `ÿ' in conditional command
status 2
-- [[ ( 'a
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: unexpected token `ÿ' in conditional command
bash: -c: line 1: expected `)'
status 2
-- [[ -f 'a
bash: -c: line 1: unexpected EOF while looking for matching `''
bash: -c: line 2: unexpected argument to conditional unary operator
status 2
-- [[ a ]] 'b
bash: -c: line 1: unexpected EOF while looking for matching `''
status 2

== status 0
