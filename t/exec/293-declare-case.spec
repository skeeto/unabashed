== stdout 205
hello HELLO
mixed case 123|MIXED CASE 123
ABC
after -l: abc
declare -l x="abc"
both: AbC
declare -- both="AbC"
plus u: abc
append: mixed case 123xyz
QUIET
existing not changed: already
reassigned: ALREADY

== stderr 0

== merged 205
hello HELLO
mixed case 123|MIXED CASE 123
ABC
after -l: abc
declare -l x="abc"
both: AbC
declare -- both="AbC"
plus u: abc
append: mixed case 123xyz
QUIET
existing not changed: already
reassigned: ALREADY

== status 0
