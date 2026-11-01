== stdout 502
default: 007 10 100 1a 7 9 B a b
name: 007 10 100 1a 7 9 B a b
-name: b a B 9 7 1a 100 10 007
+name: 007 10 100 1a 7 9 B a b
empty: 007 10 100 1a 7 9 B a b
+: 007 10 100 1a 7 9 B a b
-: b a B 9 7 1a 100 10 007
numeric: 007 7 9 10 100 1a B a b
-numeric: b a B 1a 100 10 9 007 7
bogus: 007 10 100 1a 7 9 B a b
-bogus: 007 10 100 1a 7 9 B a b
 -name: b a B 9 7 1a 100 10 007
name : 007 10 100 1a 7 9 B a b
nosort: 10 100 1a
unset: 007 10 100 1a 7 9 B a b
numeric paths: d/11 d/2 007 7 9 10 100 1a B a b d

== stderr 0

== merged 502
default: 007 10 100 1a 7 9 B a b
name: 007 10 100 1a 7 9 B a b
-name: b a B 9 7 1a 100 10 007
+name: 007 10 100 1a 7 9 B a b
empty: 007 10 100 1a 7 9 B a b
+: 007 10 100 1a 7 9 B a b
-: b a B 9 7 1a 100 10 007
numeric: 007 7 9 10 100 1a B a b
-numeric: b a B 1a 100 10 9 007 7
bogus: 007 10 100 1a 7 9 B a b
-bogus: 007 10 100 1a 7 9 B a b
 -name: b a B 9 7 1a 100 10 007
name : 007 10 100 1a 7 9 B a b
nosort: 10 100 1a
unset: 007 10 100 1a 7 9 B a b
numeric paths: d/11 d/2 007 7 9 10 100 1a B a b d

== status 0
