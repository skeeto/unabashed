== stdout 32
a b
a b c

it's
1 a b 
untraced

== stderr 117
+ echo a b
+ echo 'a b' c
+ echo ''
+ echo 'it'\''s'
+ x=1
+ y='a b'
+ z=
+ echo 1 'a b' ''
+ : noop
+ true
+ set +x

== merged 149
+ echo a b
a b
+ echo 'a b' c
a b c
+ echo ''

+ echo 'it'\''s'
it's
+ x=1
+ y='a b'
+ z=
+ echo 1 'a b' ''
1 a b 
+ : noop
+ true
+ set +x
untraced

== status 0
