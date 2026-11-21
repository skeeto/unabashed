== stdout 459
dbg [set -T]
dbg [echo a]
dbg [cat > /dev/null]
dbg [wait]
dbg [true]
dbg [false]
dbg [cat < /dev/null]
dbg [wait]
dbg [cat > /dev/null]
dbg [wait]
dbg [f]
dbg [cat]
dbg [wait]
dbg [true]
dbg [f > /dev/null]
dbg [wait]
dbg [shopt -s lastpipe]
dbg [true]
dbg [echo "lastpipe $BASH_SUBSHELL" >> log]
dbg [wait]
dbg [set +T]
dbg [echo a]
dbg [f]
dbg [wait]
dbg [trap - DEBUG]
  job [true] 1
  job [f] 1
  job [:] 1
  job [f > /dev/null] 1
  job [:] 1
lastpipe 1

== stderr 0

== merged 459
dbg [set -T]
dbg [echo a]
dbg [cat > /dev/null]
dbg [wait]
dbg [true]
dbg [false]
dbg [cat < /dev/null]
dbg [wait]
dbg [cat > /dev/null]
dbg [wait]
dbg [f]
dbg [cat]
dbg [wait]
dbg [true]
dbg [f > /dev/null]
dbg [wait]
dbg [shopt -s lastpipe]
dbg [true]
dbg [echo "lastpipe $BASH_SUBSHELL" >> log]
dbg [wait]
dbg [set +T]
dbg [echo a]
dbg [f]
dbg [wait]
dbg [trap - DEBUG]
  job [true] 1
  job [f] 1
  job [:] 1
  job [f > /dev/null] 1
  job [:] 1
lastpipe 1

== status 0
