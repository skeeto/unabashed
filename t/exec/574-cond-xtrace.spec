== stdout 188
+ p='a*'
+ s='x y'
+ [[ abc == a* ]]
+ [[ abc == a\* ]]
+ [[ x y == x y ]]
+ [[ -n x y ]]
+ [[ ! -z x y ]]
+ [[ 1+1 -eq 2 ]]
+ [[ a < b ]]
+ test -n 'x y'
+ '[' 'x y' = 'x y' ']'
+ set +x

== stderr 0

== merged 188
+ p='a*'
+ s='x y'
+ [[ abc == a* ]]
+ [[ abc == a\* ]]
+ [[ x y == x y ]]
+ [[ -n x y ]]
+ [[ ! -z x y ]]
+ [[ 1+1 -eq 2 ]]
+ [[ a < b ]]
+ test -n 'x y'
+ '[' 'x y' = 'x y' ']'
+ set +x

== status 0
