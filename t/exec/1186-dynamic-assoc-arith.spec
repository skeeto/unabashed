== stdout 2184
-- set -u; echo $((BASH_ALIASES[x]+0)) $((BASH_CMDS[x]+0))
0 0
status 0
-- alias x=1; echo $((BASH_ALIASES[x]+0)) $((BASH_ALIASES[y]+0))
1 0
status 0
-- alias x=3; y=x; echo $((BASH_ALIASES[$y]*2)) $((BASH_ALIASES[a b]+0))
6 0
status 0
-- hash -p 5 t; echo $((BASH_CMDS[t]+0))
5
status 0
-- hash -p /bin/true t; echo $((BASH_CMDS[t]+0))
bash: line 1: /bin/true: arithmetic syntax error: operand expected (error token is "/bin/true")
status 1
-- x=5; alias x=1; echo $((BASH_ALIASES[x])) $((BASH_ALIASES[1+0]+0))
1 0
status 0
-- alias x=1; let 'BASH_ALIASES[x]+=1'; alias x
alias x='2'
status 0
-- alias x=1; echo $(( BASH_ALIASES[x]++ )); alias x
1
alias x='2'
status 0
-- alias x=1; [[ -v BASH_ALIASES[x] ]]; echo $?; [[ -v BASH_ALIASES[y] ]]; echo $?
0
1
status 0
-- alias x=1; shopt -s array_expand_once; [[ -v BASH_ALIASES[x] ]]; echo $?
0
status 0
-- alias 0=7; echo "<$BASH_ALIASES>" $((BASH_ALIASES)) ${#BASH_ALIASES} ${BASH_ALIASES:0:1}
<7> 7 1 7
status 0
-- alias 0=7; x=BASH_ALIASES; echo ${!x} $((x))
7 7
status 0
-- hash -p 5 0; echo "<$BASH_CMDS>" $((BASH_CMDS))
<5> 5
status 0
-- [[ -v BASH_ALIASES ]]; echo $?; alias 0=7; [[ -v BASH_ALIASES ]]; echo $?
1
0
status 0
-- set -u; echo "<$BASH_ALIASES>"
bash: line 1: BASH_ALIASES: unbound variable
status 127
-- alias 0=7; BASH_ALIASES=8; alias 0; read -r BASH_ALIASES <<<9; alias 0
alias 0='8'
alias 0='9'
status 0
-- alias 0=1; BASH_ALIASES+=x; alias 0; (( BASH_ALIASES=3 )); alias 0
alias 0='x'
alias 0='3'
status 0
-- BASH_CMDS=/bin/ls; hash; echo "<$BASH_CMDS>"
hits	command
   0	/bin/ls
</bin/ls>
status 0
-- f() { local BASH_ALIASES=3; echo "<$BASH_ALIASES>"; alias; }; f
<3>
status 0
-- cd /; pushd /tmp >/dev/null; echo "<$DIRSTACK>"; echo $((DIRSTACK))
</tmp>
bash: line 1: /tmp: arithmetic syntax error: operand expected (error token is "/tmp")
status 1
-- declare -A m; echo $((m[a b]+0)) $((m[x]=3)) ${m[x]}
0 3 3
status 0
-- set -u; declare -A m=([a]=1); a=([1]=1); echo $((m)) $((a)) $((BASH_ALIASES))
0 0 0
status 0
-- set -u; f() { echo $((BASH_ARGV)) $((BASH_SOURCE)); }; f
bash: line 1: bash: unbound variable
status 127
-- set -u; declare -a e; echo $((e))
bash: line 1: e: unbound variable
status 127

== stderr 0

== merged 2184
-- set -u; echo $((BASH_ALIASES[x]+0)) $((BASH_CMDS[x]+0))
0 0
status 0
-- alias x=1; echo $((BASH_ALIASES[x]+0)) $((BASH_ALIASES[y]+0))
1 0
status 0
-- alias x=3; y=x; echo $((BASH_ALIASES[$y]*2)) $((BASH_ALIASES[a b]+0))
6 0
status 0
-- hash -p 5 t; echo $((BASH_CMDS[t]+0))
5
status 0
-- hash -p /bin/true t; echo $((BASH_CMDS[t]+0))
bash: line 1: /bin/true: arithmetic syntax error: operand expected (error token is "/bin/true")
status 1
-- x=5; alias x=1; echo $((BASH_ALIASES[x])) $((BASH_ALIASES[1+0]+0))
1 0
status 0
-- alias x=1; let 'BASH_ALIASES[x]+=1'; alias x
alias x='2'
status 0
-- alias x=1; echo $(( BASH_ALIASES[x]++ )); alias x
1
alias x='2'
status 0
-- alias x=1; [[ -v BASH_ALIASES[x] ]]; echo $?; [[ -v BASH_ALIASES[y] ]]; echo $?
0
1
status 0
-- alias x=1; shopt -s array_expand_once; [[ -v BASH_ALIASES[x] ]]; echo $?
0
status 0
-- alias 0=7; echo "<$BASH_ALIASES>" $((BASH_ALIASES)) ${#BASH_ALIASES} ${BASH_ALIASES:0:1}
<7> 7 1 7
status 0
-- alias 0=7; x=BASH_ALIASES; echo ${!x} $((x))
7 7
status 0
-- hash -p 5 0; echo "<$BASH_CMDS>" $((BASH_CMDS))
<5> 5
status 0
-- [[ -v BASH_ALIASES ]]; echo $?; alias 0=7; [[ -v BASH_ALIASES ]]; echo $?
1
0
status 0
-- set -u; echo "<$BASH_ALIASES>"
bash: line 1: BASH_ALIASES: unbound variable
status 127
-- alias 0=7; BASH_ALIASES=8; alias 0; read -r BASH_ALIASES <<<9; alias 0
alias 0='8'
alias 0='9'
status 0
-- alias 0=1; BASH_ALIASES+=x; alias 0; (( BASH_ALIASES=3 )); alias 0
alias 0='x'
alias 0='3'
status 0
-- BASH_CMDS=/bin/ls; hash; echo "<$BASH_CMDS>"
hits	command
   0	/bin/ls
</bin/ls>
status 0
-- f() { local BASH_ALIASES=3; echo "<$BASH_ALIASES>"; alias; }; f
<3>
status 0
-- cd /; pushd /tmp >/dev/null; echo "<$DIRSTACK>"; echo $((DIRSTACK))
</tmp>
bash: line 1: /tmp: arithmetic syntax error: operand expected (error token is "/tmp")
status 1
-- declare -A m; echo $((m[a b]+0)) $((m[x]=3)) ${m[x]}
0 3 3
status 0
-- set -u; declare -A m=([a]=1); a=([1]=1); echo $((m)) $((a)) $((BASH_ALIASES))
0 0 0
status 0
-- set -u; f() { echo $((BASH_ARGV)) $((BASH_SOURCE)); }; f
bash: line 1: bash: unbound variable
status 127
-- set -u; declare -a e; echo $((e))
bash: line 1: e: unbound variable
status 127

== status 0
