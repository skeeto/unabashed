== stdout 33
prefix
1 2 x y it's 7 sub 7 more

== stderr 269
+ a=1
+ b=2
+ s='x y'
+ q='it'\''s'
+ t=$'tab\there'
+ n=7
++ echo sub
+ c=sub
+ a=5
+ echo prefix
+ a=6
+ b=7
+ : multi
+ export e=1
+ e=1
+ readonly 'r=a b'
+ r='a b'
+ declare -i i=3+4
+ local_test
+ local l=v m
+ l=w
+ x+=more
+ echo '1 2 x y it'\''s 7 sub 7 more'

== merged 302
+ a=1
+ b=2
+ s='x y'
+ q='it'\''s'
+ t=$'tab\there'
+ n=7
++ echo sub
+ c=sub
+ a=5
+ echo prefix
prefix
+ a=6
+ b=7
+ : multi
+ export e=1
+ e=1
+ readonly 'r=a b'
+ r='a b'
+ declare -i i=3+4
+ local_test
+ local l=v m
+ l=w
+ x+=more
+ echo '1 2 x y it'\''s 7 sub 7 more'
1 2 x y it's 7 sub 7 more

== status 0
