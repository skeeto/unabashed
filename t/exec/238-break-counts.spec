== stdout 102
1a
2a
3a
--
1a
after break 2: 0
1a
2a
--
break 99: 0 1a
1a
2a
continue 99: 0
break 1: 1
continue 1: 2

== stderr 0

== merged 102
1a
2a
3a
--
1a
after break 2: 0
1a
2a
--
break 99: 0 1a
1a
2a
continue 99: 0
break 1: 1
continue 1: 2

== status 0
