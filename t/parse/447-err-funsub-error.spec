== stdout 2
a

== stderr 127
bash: -c: line 2: syntax error near unexpected token `esac' while looking for matching `}'
bash: -c: line 2: `echo ${ esac; }'

== merged 129
a
bash: -c: line 2: syntax error near unexpected token `esac' while looking for matching `}'
bash: -c: line 2: `echo ${ esac; }'

== status 127
