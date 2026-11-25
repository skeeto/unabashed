== stdout 19
1: 1
yes
2: 0
3: 1

== stderr 333
bash: command substitution: line 11: syntax error near unexpected token `}' while looking for matching `)'
bash: command substitution: line 11: ` echo yes { y=1; })"'
bash: command substitution: line 30: syntax error near unexpected token `}' while looking for matching `)'
bash: command substitution: line 30: ` echo yes { y=1; })'

== merged 352
bash: command substitution: line 11: syntax error near unexpected token `}' while looking for matching `)'
bash: command substitution: line 11: ` echo yes { y=1; })"'
1: 1
yes
2: 0
bash: command substitution: line 30: syntax error near unexpected token `}' while looking for matching `)'
bash: command substitution: line 30: ` echo yes { y=1; })'
3: 1

== status 0
