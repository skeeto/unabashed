== stdout 571
dbg1
dbg1
dbg2
dbg2
dbg2
err2
dbg2
in-s1
dbg2
ret2
dbg2
trap -- 'echo err2' ERR
trap -- 'echo ret2' RETURN
-- the caller's, listed and run
dbg1
trap -- 'echo err1' ERR
trap -- 'echo ret1' RETURN
err1
in-s2
ret1
dbg1
-- the same text
dbg1
dbg1
in-s3
dbg1
top
dbg1
-- unset in the file, and put back
dbg1
dbg1
in-s4
dbg1
top
dbg1
-- in a function
dbgf
trap -- 'echo retf' RETURN
in-s5
retf
dbgf
f-after
dbgf
retf
d echo in-f
in-f
d echo top
top
d trap - DEBUG
-- set -T
d . ./s2.sh
d trap -p
trap -- 'echo "d $BASH_COMMAND"' DEBUG
d false
d echo in-s2
in-s2
d trap - DEBUG

== stderr 0

== merged 571
dbg1
dbg1
dbg2
dbg2
dbg2
err2
dbg2
in-s1
dbg2
ret2
dbg2
trap -- 'echo err2' ERR
trap -- 'echo ret2' RETURN
-- the caller's, listed and run
dbg1
trap -- 'echo err1' ERR
trap -- 'echo ret1' RETURN
err1
in-s2
ret1
dbg1
-- the same text
dbg1
dbg1
in-s3
dbg1
top
dbg1
-- unset in the file, and put back
dbg1
dbg1
in-s4
dbg1
top
dbg1
-- in a function
dbgf
trap -- 'echo retf' RETURN
in-s5
retf
dbgf
f-after
dbgf
retf
d echo in-f
in-f
d echo top
top
d trap - DEBUG
-- set -T
d . ./s2.sh
d trap -p
trap -- 'echo "d $BASH_COMMAND"' DEBUG
d false
d echo in-s2
in-s2
d trap - DEBUG

== status 0
