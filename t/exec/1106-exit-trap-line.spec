== stdout 4480
-- echo one true
one
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
one
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- echo one true  
one
./s.sh: line 6: BASH_TRAPSIG: readonly variable
./s.sh: line 6: BASH_TRAPSIG: readonly variable
one
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
one
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
-- cat <<E x E
x
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
x
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- f() { : } f
./s.sh: line 6: BASH_TRAPSIG: readonly variable
./s.sh: line 6: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- if true; then exit fi
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- eval "  exit 4" echo no
./s.sh: line 2: BASH_TRAPSIG: readonly variable
./s.sh: line 2: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- f() {  eval " exit" } f
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e while true; do false done 
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e f() { if true; then false fi }  f
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e for i in 1; do : false done
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e case x in x) false esac
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e f() { : false } f
./s.sh: line 4: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e x=$( exit 3 )
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- eval " set -e false"
./s.sh: line 2: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
--  . ./src.sh
./src.sh: line 3: BASH_TRAPSIG: readonly variable
./src.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- x=$( trap : EXIT true )
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- ( trap : EXIT true )
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- ( trap : EXIT exit 2 )
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: line 3: BASH_TRAPSIG: readonly variable
bash: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e; readonly q  if read q <<<x; then :; fi
./s.sh: line 4: q: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
bash: line 4: q: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
[] [./src.sh ./s.sh] [s1 s2]
[] [] [s1 s2]
[] [./s.sh] [s1 s2]
[] [] [s1 s2]
[source f main] [./src.sh ./s.sh ./s.sh] [s1 s2]
[source f] [./src.sh bash] [s1 s2]
bash: trap: line 2: syntax error: unexpected end of file from `if' command on line 1
bash: error trap: line 2: syntax error: unexpected end of file from `if' command on line 1
bash: return trap: line 3: syntax error: unexpected end of file from `if' command on line 2
bash: debug trap: line 2: syntax error near unexpected token `fi'
bash: debug trap: line 2: `fi'
bash: exit trap: line 2: syntax error: unexpected end of file from `if' command on line 1

== stderr 0

== merged 4480
-- echo one true
one
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
one
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- echo one true  
one
./s.sh: line 6: BASH_TRAPSIG: readonly variable
./s.sh: line 6: BASH_TRAPSIG: readonly variable
one
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
one
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
-- cat <<E x E
x
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
x
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- f() { : } f
./s.sh: line 6: BASH_TRAPSIG: readonly variable
./s.sh: line 6: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- if true; then exit fi
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- eval "  exit 4" echo no
./s.sh: line 2: BASH_TRAPSIG: readonly variable
./s.sh: line 2: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- f() {  eval " exit" } f
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e while true; do false done 
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e f() { if true; then false fi }  f
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e for i in 1; do : false done
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e case x in x) false esac
./s.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e f() { : false } f
./s.sh: line 4: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e x=$( exit 3 )
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- eval " set -e false"
./s.sh: line 2: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
--  . ./src.sh
./src.sh: line 3: BASH_TRAPSIG: readonly variable
./src.sh: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- x=$( trap : EXIT true )
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- ( trap : EXIT true )
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- ( trap : EXIT exit 2 )
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 3: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
./s.sh: line 5: BASH_TRAPSIG: readonly variable
bash: line 3: BASH_TRAPSIG: readonly variable
bash: line 3: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
bash: BASH_TRAPSIG: readonly variable
-- set -e; readonly q  if read q <<<x; then :; fi
./s.sh: line 4: q: readonly variable
./s.sh: line 4: BASH_TRAPSIG: readonly variable
bash: line 4: q: readonly variable
bash: line 4: BASH_TRAPSIG: readonly variable
[] [./src.sh ./s.sh] [s1 s2]
[] [] [s1 s2]
[] [./s.sh] [s1 s2]
[] [] [s1 s2]
[source f main] [./src.sh ./s.sh ./s.sh] [s1 s2]
[source f] [./src.sh bash] [s1 s2]
bash: trap: line 2: syntax error: unexpected end of file from `if' command on line 1
bash: error trap: line 2: syntax error: unexpected end of file from `if' command on line 1
bash: return trap: line 3: syntax error: unexpected end of file from `if' command on line 2
bash: debug trap: line 2: syntax error near unexpected token `fi'
bash: debug trap: line 2: `fi'
bash: exit trap: line 2: syntax error: unexpected end of file from `if' command on line 1

== status 0
