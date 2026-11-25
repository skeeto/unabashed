== stdout 2719
-- true {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- sleep 0 {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- ( : ) {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- exec {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- true {GROUPS[1]}>/dev/null; echo $?
bash: line 1: GROUPS[1]: cannot assign fd to variable
1
status 0
-- declare -n r=GROUPS; true {r}>/dev/null; echo $?
bash: line 1: r: cannot assign fd to variable
1
status 0
-- f() { true {FUNCNAME}>/dev/null; echo $?; }; f
bash: line 1: FUNCNAME: cannot assign fd to variable
1
status 0
-- true {BASH_SOURCE}>/dev/null; true {BASH_LINENO}>/dev/null; true {BASH_ARGV}>/dev/null; sleep 0 {BASH_ARGC}>/dev/null
bash: line 1: BASH_SOURCE: cannot assign fd to variable
bash: line 1: BASH_LINENO: cannot assign fd to variable
bash: line 1: BASH_ARGV: cannot assign fd to variable
bash: line 1: BASH_ARGC: cannot assign fd to variable
status 1
-- true {x}>/dev/null {GROUPS}>/dev/null {y}>/dev/null; echo $? ${x-unset} ${y-unset}
bash: line 1: GROUPS: cannot assign fd to variable
1 10 unset
status 0
-- RANDOM=1; a=$RANDOM; RANDOM=1; sleep 0 {RANDOM}>/dev/null; b=$RANDOM; [ $a = $b ] && echo same
same
status 0
-- SECONDS=100; sleep 0 {SECONDS}>/dev/null; echo $SECONDS
100
status 0
-- SECONDS=100; ( echo $SECONDS ) {SECONDS}>/dev/null; echo $SECONDS
10
100
status 0
-- sleep 0 {BASH_ARGV0}>/dev/null; echo $0 $BASH_ARGV0
bash bash
status 0
-- ( echo $BASH_ARGV0 ) {BASH_ARGV0}>/dev/null; echo $BASH_ARGV0
10
bash
status 0
-- sleep 0 {BASH_SUBSHELL}>/dev/null; echo $BASH_SUBSHELL
0
status 0
-- sleep 0 {COMP_WORDBREAKS}>/dev/null; echo "<$COMP_WORDBREAKS>"
< 	
"'@><=;|&(:>
status 0
-- ( echo "<$COMP_WORDBREAKS>" ) {COMP_WORDBREAKS}>/dev/null
<10>
status 0
-- sleep 0 {BASH_ALIASES}>/dev/null {BASH_ALIASES[x]}>/dev/null; alias; echo "<$BASH_ALIASES>"
<>
status 0
-- ( echo $BASH_ALIASES ) {BASH_ALIASES}>/dev/null; alias
10
status 0
-- sleep 0 {BASH_CMDS[q]}>/dev/null; hash
hits	command
   1	/opt/fakes/bin/sleep
status 0
-- true {BASH_ALIASES[x]}>/dev/null; alias
alias x='10'
status 0
-- ( alias ) {BASH_ALIASES[x]}>/dev/null; alias
alias x='10'
status 0
-- ( alias; echo "<$BASH_ALIASES>" ) {BASH_ALIASES}>/dev/null {BASH_ALIASES[y]}>/dev/null
alias 0='10'
alias y='11'
<10>
status 0
-- ( hash -t q ) {BASH_CMDS[q]}>/dev/null
10
status 0
-- ( echo $0 $BASH_SUBSHELL; ( echo $0 $BASH_SUBSHELL ) ) {BASH_ARGV0}>/dev/null {BASH_SUBSHELL}>/dev/null; echo $0 $BASH_SUBSHELL
10 11
10 12
bash 0
status 0
-- RANDOM=10; a=$RANDOM; ( echo $(( RANDOM == a )) ) {RANDOM}>/dev/null
1
status 0

== stderr 0

== merged 2719
-- true {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- sleep 0 {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- ( : ) {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- exec {GROUPS}>/dev/null; echo $?
bash: line 1: GROUPS: cannot assign fd to variable
1
status 0
-- true {GROUPS[1]}>/dev/null; echo $?
bash: line 1: GROUPS[1]: cannot assign fd to variable
1
status 0
-- declare -n r=GROUPS; true {r}>/dev/null; echo $?
bash: line 1: r: cannot assign fd to variable
1
status 0
-- f() { true {FUNCNAME}>/dev/null; echo $?; }; f
bash: line 1: FUNCNAME: cannot assign fd to variable
1
status 0
-- true {BASH_SOURCE}>/dev/null; true {BASH_LINENO}>/dev/null; true {BASH_ARGV}>/dev/null; sleep 0 {BASH_ARGC}>/dev/null
bash: line 1: BASH_SOURCE: cannot assign fd to variable
bash: line 1: BASH_LINENO: cannot assign fd to variable
bash: line 1: BASH_ARGV: cannot assign fd to variable
bash: line 1: BASH_ARGC: cannot assign fd to variable
status 1
-- true {x}>/dev/null {GROUPS}>/dev/null {y}>/dev/null; echo $? ${x-unset} ${y-unset}
bash: line 1: GROUPS: cannot assign fd to variable
1 10 unset
status 0
-- RANDOM=1; a=$RANDOM; RANDOM=1; sleep 0 {RANDOM}>/dev/null; b=$RANDOM; [ $a = $b ] && echo same
same
status 0
-- SECONDS=100; sleep 0 {SECONDS}>/dev/null; echo $SECONDS
100
status 0
-- SECONDS=100; ( echo $SECONDS ) {SECONDS}>/dev/null; echo $SECONDS
10
100
status 0
-- sleep 0 {BASH_ARGV0}>/dev/null; echo $0 $BASH_ARGV0
bash bash
status 0
-- ( echo $BASH_ARGV0 ) {BASH_ARGV0}>/dev/null; echo $BASH_ARGV0
10
bash
status 0
-- sleep 0 {BASH_SUBSHELL}>/dev/null; echo $BASH_SUBSHELL
0
status 0
-- sleep 0 {COMP_WORDBREAKS}>/dev/null; echo "<$COMP_WORDBREAKS>"
< 	
"'@><=;|&(:>
status 0
-- ( echo "<$COMP_WORDBREAKS>" ) {COMP_WORDBREAKS}>/dev/null
<10>
status 0
-- sleep 0 {BASH_ALIASES}>/dev/null {BASH_ALIASES[x]}>/dev/null; alias; echo "<$BASH_ALIASES>"
<>
status 0
-- ( echo $BASH_ALIASES ) {BASH_ALIASES}>/dev/null; alias
10
status 0
-- sleep 0 {BASH_CMDS[q]}>/dev/null; hash
hits	command
   1	/opt/fakes/bin/sleep
status 0
-- true {BASH_ALIASES[x]}>/dev/null; alias
alias x='10'
status 0
-- ( alias ) {BASH_ALIASES[x]}>/dev/null; alias
alias x='10'
status 0
-- ( alias; echo "<$BASH_ALIASES>" ) {BASH_ALIASES}>/dev/null {BASH_ALIASES[y]}>/dev/null
alias 0='10'
alias y='11'
<10>
status 0
-- ( hash -t q ) {BASH_CMDS[q]}>/dev/null
10
status 0
-- ( echo $0 $BASH_SUBSHELL; ( echo $0 $BASH_SUBSHELL ) ) {BASH_ARGV0}>/dev/null {BASH_SUBSHELL}>/dev/null; echo $0 $BASH_SUBSHELL
10 11
10 12
bash 0
status 0
-- RANDOM=10; a=$RANDOM; ( echo $(( RANDOM == a )) ) {RANDOM}>/dev/null
1
status 0

== status 0
