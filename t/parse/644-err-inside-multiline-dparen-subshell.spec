== stdout 2
a

== stderr 101
bash: -c: line 5: syntax error near unexpected token `fi'
bash: -c: line 5: `(echo a
  echo b
  fi)'

== merged 103
a
bash: -c: line 5: syntax error near unexpected token `fi'
bash: -c: line 5: `(echo a
  echo b
  fi)'

== status 2
