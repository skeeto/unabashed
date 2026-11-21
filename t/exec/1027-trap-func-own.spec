== stdout 523
err2
f done
err2
err2
err2
err1
same done
-- unset in a function, and put back
unset done
err1
-- a callee's sets for its caller; its unset is undone
errg
f done
errf
errf
errf
trap -- 'echo errf' ERR
-- listed only where it runs
listed
-- DEBUG
outer-dbg
inner-dbg
in-f
inner-dbg
in-f2
inner-dbg
dbg declare -ft g
dbg f
in-g
in-g
in-f
dbg set +T
dbg trap - DEBUG
-- RETURN
in-f
ret2
in-g
ret-g g
in-f
ret-g f
newret
after-debug
trap -- 'echo newret' RETURN
-- recursion
err1
err1
err1
err1
err1
-- exit in a function
EXIT

== stderr 0

== merged 523
err2
f done
err2
err2
err2
err1
same done
-- unset in a function, and put back
unset done
err1
-- a callee's sets for its caller; its unset is undone
errg
f done
errf
errf
errf
trap -- 'echo errf' ERR
-- listed only where it runs
listed
-- DEBUG
outer-dbg
inner-dbg
in-f
inner-dbg
in-f2
inner-dbg
dbg declare -ft g
dbg f
in-g
in-g
in-f
dbg set +T
dbg trap - DEBUG
-- RETURN
in-f
ret2
in-g
ret-g g
in-f
ret-g f
newret
after-debug
trap -- 'echo newret' RETURN
-- recursion
err1
err1
err1
err1
err1
-- exit in a function
EXIT

== status 3
