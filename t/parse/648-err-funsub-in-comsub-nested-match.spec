== stdout 2
a

== stderr 157
bash: -c: line 2: syntax error near unexpected token `esac' while looking for matching `}'
bash: -c: line 2: `f() { x=$(echo ${ case y in esac; esac; }); }'

== merged 159
a
bash: -c: line 2: syntax error near unexpected token `esac' while looking for matching `}'
bash: -c: line 2: `f() { x=$(echo ${ case y in esac; esac; }); }'

== status 127
