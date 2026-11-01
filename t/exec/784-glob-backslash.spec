== stdout 165
a\*b: a*b
a\*: a*b
a\?: a?
a\b: ab
a\b: ab
$v: a\*
$v: a\b
$v*: ab
$v w: a\*
$v a: a*b a? aXb a\b ab
trailing: a*\
case escaped
case quoted
case var
case var escape

== stderr 0

== merged 165
a\*b: a*b
a\*: a*b
a\?: a?
a\b: ab
a\b: ab
$v: a\*
$v: a\b
$v*: ab
$v w: a\*
$v a: a*b a? aXb a\b ab
trailing: a*\
case escaped
case quoted
case var
case var escape

== status 0
