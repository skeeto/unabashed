== stdout 0

== stderr 220
+ for i in a 'b c'
+ :
+ for i in a 'b c'
+ :
+ case x in
+ : matched
+ case "a b" in
+ :
+ true
+ : yes
+ false
+ true
+ : brace
+ false
+ true
+ : and
+ false
+ : or
+ i=0
+ ((  i < 1  ))
+ ((  i++  ))
+ ((  i < 1  ))

== merged 220
+ for i in a 'b c'
+ :
+ for i in a 'b c'
+ :
+ case x in
+ : matched
+ case "a b" in
+ :
+ true
+ : yes
+ false
+ true
+ : brace
+ false
+ true
+ : and
+ false
+ : or
+ i=0
+ ((  i < 1  ))
+ ((  i++  ))
+ ((  i < 1  ))

== status 1
