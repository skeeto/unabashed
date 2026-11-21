== stdout 429
exit [cat > /dev/null]
exit [echo a]
exit [cat > /dev/null]
exit [cat]
exit [:]
exit [cat]
exit [f > /dev/null]
exit [cat > /dev/null]
exit [cat > /dev/null]
exit [echo a > /dev/null]
exit [true]
exit [cat > /dev/null]
exit [read v]
0
usr1 [tr -d " "]
-- DEBUG
dbg [echo a]
dbg [cat > /dev/null]
dbg [wait]
dbg [echo a]
dbg [cat 2> /dev/null]
dbg [cat > /dev/null]
dbg [wait]
-- interactive
in
sig=[10]
usr2 [kill -USR2 $$]
done

== stderr 0

== merged 429
exit [cat > /dev/null]
exit [echo a]
exit [cat > /dev/null]
exit [cat]
exit [:]
exit [cat]
exit [f > /dev/null]
exit [cat > /dev/null]
exit [cat > /dev/null]
exit [echo a > /dev/null]
exit [true]
exit [cat > /dev/null]
exit [read v]
0
usr1 [tr -d " "]
-- DEBUG
dbg [echo a]
dbg [cat > /dev/null]
dbg [wait]
dbg [echo a]
dbg [cat 2> /dev/null]
dbg [cat > /dev/null]
dbg [wait]
-- interactive
in
sig=[10]
usr2 [kill -USR2 $$]
done

== status 0
