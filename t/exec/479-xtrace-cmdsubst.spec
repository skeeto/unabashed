== stdout 42
hi
a
b
three
paren
paren2
infunc
ev
evsub

== stderr 334
++ echo hi
+ echo hi
+++ echo deep
++ echo deep
+ x=deep
++ echo a
++ echo b
+ echo $'a\nb'
++ echo bq
+ y=bq
++++ echo three
+++ echo three
++ echo three
+ echo three
++ echo sub
+ z=sub
+ echo paren
+ echo paren2
++ f
++ echo infunc
+ echo infunc
+ eval 'echo ev'
++ echo ev
+ eval 'echo $(echo evsub)'
+++ echo evsub
++ echo evsub

== merged 376
++ echo hi
+ echo hi
hi
+++ echo deep
++ echo deep
+ x=deep
++ echo a
++ echo b
+ echo $'a\nb'
a
b
++ echo bq
+ y=bq
++++ echo three
+++ echo three
++ echo three
+ echo three
three
++ echo sub
+ z=sub
+ echo paren
paren
+ echo paren2
paren2
++ f
++ echo infunc
+ echo infunc
infunc
+ eval 'echo ev'
++ echo ev
ev
+ eval 'echo $(echo evsub)'
+++ echo evsub
++ echo evsub
evsub

== status 0
