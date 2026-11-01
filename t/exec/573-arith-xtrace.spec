== stdout 194
+ x=3
+ ((  x += 2  ))
+ ((  x > 100  ))
+ let 'y = x * 2' z=1
+ (( i = 0 ))
+ (( i < 2 ))
+ :
+ (( i++  ))
+ (( i < 2 ))
+ :
+ (( i++  ))
+ (( i < 2 ))
+ echo 15
15
+ declare -i d=1+1
+ set +x

== stderr 0

== merged 194
+ x=3
+ ((  x += 2  ))
+ ((  x > 100  ))
+ let 'y = x * 2' z=1
+ (( i = 0 ))
+ (( i < 2 ))
+ :
+ (( i++  ))
+ (( i < 2 ))
+ :
+ (( i++  ))
+ (( i < 2 ))
+ echo 15
15
+ declare -i d=1+1
+ set +x

== status 0
