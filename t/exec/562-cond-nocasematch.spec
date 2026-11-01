== stdout 161
F: [[ ABC == abc ]]
T: [[ ABC == abc ]]
T: [[ ABC == a* ]]
F: [[ ABC != abc ]]
T: [[ X == [a-z] ]]
T: [[ ABC < abd ]]
case-match
F: [[ ABC == a* ]]
case-nomatch

== stderr 0

== merged 161
F: [[ ABC == abc ]]
T: [[ ABC == abc ]]
T: [[ ABC == a* ]]
F: [[ ABC != abc ]]
T: [[ X == [a-z] ]]
T: [[ ABC < abd ]]
case-match
F: [[ ABC == a* ]]
case-nomatch

== status 0
