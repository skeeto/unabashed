== stdout 300
debug 1
s1
s2
debug 1
end
---
debug 1
debug 1
debug 1
s1
debug 2
s2
debug 1
end
---
s1
s2
ret 2 x x
end
---
d2 2
t1
d2 3
t2
d2 1
end
---
debug 1
debug 1
s1
s2
debug 1
debug 1
debug 1
debug 1
debug 1
s1
debug 2
s2
debug 1
ret 1
debug 1
ret 1
---
in f: f source g ./sub3.sh ./sub3.sh bash
exit: 1
st 3

== stderr 0

== merged 300
debug 1
s1
s2
debug 1
end
---
debug 1
debug 1
debug 1
s1
debug 2
s2
debug 1
end
---
s1
s2
ret 2 x x
end
---
d2 2
t1
d2 3
t2
d2 1
end
---
debug 1
debug 1
s1
s2
debug 1
debug 1
debug 1
debug 1
debug 1
s1
debug 2
s2
debug 1
ret 1
debug 1
ret 1
---
in f: f source g ./sub3.sh ./sub3.sh bash
exit: 1
st 3

== status 0
