== stdout 9
body
end

== stderr 112
bash: -c: line 1: syntax error near unexpected token `;'
bash: -c: line 1: `f() { :; } <<"echo end"; a=(1 ; 2)'

== merged 121
bash: -c: line 1: syntax error near unexpected token `;'
bash: -c: line 1: `f() { :; } <<"echo end"; a=(1 ; 2)'
body
end

== status 0
