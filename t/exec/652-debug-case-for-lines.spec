== stdout 488
debug 4 [case x$((1)) in ]
debug 5 [echo one]
one
debug 7 [for i in 1 2]
debug 8 [case $i in ]
debug 7 [for i in 1 2]
debug 8 [case $i in ]
debug 9 [echo two]
two
debug 12 [select s in a]

debug 13 [false]
debug 14 [case y in ]
debug 14 [true]
debug 14 [echo yes]
yes
debug 15 [for i in 0 1]
debug 16 [for j in 3 4]
debug 17 [: $i $j]
debug 16 [for j in 3 4]
debug 17 [: $i $j]
debug 15 [for i in 0 1]
debug 16 [for j in 3 4]
debug 17 [: $i $j]
debug 16 [for j in 3 4]
debug 17 [: $i $j]

== stderr 0

== merged 488
debug 4 [case x$((1)) in ]
debug 5 [echo one]
one
debug 7 [for i in 1 2]
debug 8 [case $i in ]
debug 7 [for i in 1 2]
debug 8 [case $i in ]
debug 9 [echo two]
two
debug 12 [select s in a]

debug 13 [false]
debug 14 [case y in ]
debug 14 [true]
debug 14 [echo yes]
yes
debug 15 [for i in 0 1]
debug 16 [for j in 3 4]
debug 17 [: $i $j]
debug 16 [for j in 3 4]
debug 17 [: $i $j]
debug 15 [for i in 0 1]
debug 16 [for j in 3 4]
debug 17 [: $i $j]
debug 16 [for j in 3 4]
debug 17 [: $i $j]

== status 0
