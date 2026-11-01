== stdout 535
top 1
debug 20  <trap 'echo "return $LINENO ${FUNCNAME[0]}"' RETURN>
debug 21  <f>
debug 6 f <f>
debug 7 f <echo body1>
body1
debug 8 f <echo body2>
body2
debug 6 f <echo body2>
return 6 f
debug 22  <g>
debug 11 g <g>
debug 12 g <echo gbody>
gbody
debug 11 g <echo gbody>
return 11 g
debug 23  <((i=0))>
debug 23  <((i<2))>
debug 23  <:>
debug 23  <((i++))>
debug 23  <((i<2))>
debug 23  <:>
debug 23  <((i++))>
debug 23  <((i<2))>
debug 24  <trap - DEBUG RETURN>
D 29
D 26
D 26
traced
D 30
15 bash
15 k bash
status 1
32 NULL
status 1

== stderr 0

== merged 535
top 1
debug 20  <trap 'echo "return $LINENO ${FUNCNAME[0]}"' RETURN>
debug 21  <f>
debug 6 f <f>
debug 7 f <echo body1>
body1
debug 8 f <echo body2>
body2
debug 6 f <echo body2>
return 6 f
debug 22  <g>
debug 11 g <g>
debug 12 g <echo gbody>
gbody
debug 11 g <echo gbody>
return 11 g
debug 23  <((i=0))>
debug 23  <((i<2))>
debug 23  <:>
debug 23  <((i++))>
debug 23  <((i<2))>
debug 23  <:>
debug 23  <((i++))>
debug 23  <((i<2))>
debug 24  <trap - DEBUG RETURN>
D 29
D 26
D 26
traced
D 30
15 bash
15 k bash
status 1
32 NULL
status 1

== status 0
