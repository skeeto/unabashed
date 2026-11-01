== stdout 104
ABCD
abcd
ui=7
declare -u u="ABCD"
declare -l l="abcd"
declare -iu ui="7"
HELLO WORLD
HELLO WORLD AGAIN

== stderr 0

== merged 104
ABCD
abcd
ui=7
declare -u u="ABCD"
declare -l l="abcd"
declare -iu ui="7"
HELLO WORLD
HELLO WORLD AGAIN

== status 0
