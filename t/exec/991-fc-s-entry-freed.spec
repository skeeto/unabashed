== stdout 208
hi there
hi there
second entry here
other entry here
third
HISTCONTROL=erasedups
echo hi there
echo second entry here
echo other entry here
HISTCONTROL=
echo third
fc -s third
history | sed 's/^ *[0-9]* *//'

== stderr 72
echo hi there
echo other entry here
bash: line 11: fc: no command found

== merged 280
hi there
echo hi there
hi there
second entry here
echo other entry here
other entry here
third
bash: line 11: fc: no command found
HISTCONTROL=erasedups
echo hi there
echo second entry here
echo other entry here
HISTCONTROL=
echo third
fc -s third
history | sed 's/^ *[0-9]* *//'

== status 0
