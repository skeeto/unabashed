== stdout 99
*/*: r/a rx/c
*/.: rx/. x/.
*/: r/ rx/ x/
r/*: r/a
x/*: x/*
x/b: x/b*
*/b: x/b
*/a: */a
rx/*: rx/c

== stderr 0

== merged 99
*/*: r/a rx/c
*/.: rx/. x/.
*/: r/ rx/ x/
r/*: r/a
x/*: x/*
x/b: x/b*
*/b: x/b
*/a: */a
rx/*: rx/c

== status 0
