== stdout 172
a[b/c]: a[b/c]
a[/]c: a[/]c
a[b/c]*: a[b/c]
a[bc]/*: ab/c
a?/*: ab/c
*[b/c]: a[b/c]
a[!x]/c: ab/c
ignore a[/]c: a/c
ignore a[a-z]c: a/c
ignore a?c: a/c
ignore a*: a/* ab/*

== stderr 0

== merged 172
a[b/c]: a[b/c]
a[/]c: a[/]c
a[b/c]*: a[b/c]
a[bc]/*: ab/c
a?/*: ab/c
*[b/c]: a[b/c]
a[!x]/c: ab/c
ignore a[/]c: a/c
ignore a[a-z]c: a/c
ignore a?c: a/c
ignore a*: a/* ab/*

== status 0
