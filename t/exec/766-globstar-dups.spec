== stdout 217
**/a/**: a a/a a/a a/a/a a/a/a a/a/a b/a
**/a: a a/a a/a/a b/a
**/a/: a/ a/a/ a/a/a/ b/a/
**/a/*: a/a a/a/a
a/**/a: a/a a/a/a
**/**/a/**: a a/a a/a a/a/a a/a/a a/a/a b/a
a/**/**/a: a/a a/a/a
*/**/a: a/a a/a/a b/a
0
7

== stderr 0

== merged 217
**/a/**: a a/a a/a a/a/a a/a/a a/a/a b/a
**/a: a a/a a/a/a b/a
**/a/: a/ a/a/ a/a/a/ b/a/
**/a/*: a/a a/a/a
a/**/a: a/a a/a/a
**/**/a/**: a a/a a/a a/a/a a/a/a a/a/a b/a
a/**/**/a: a/a a/a/a
*/**/a: a/a a/a/a b/a
0
7

== status 0
