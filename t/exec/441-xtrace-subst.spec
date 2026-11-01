== stdout 15
sub
evald
done

== stderr 140
++ echo inner
+ x=inner
+ echo sub
+++ echo deep
++ echo deep
+ y=deep
+ eval 'echo evald'
++ echo evald
++ echo funsub
+ z=funsub
+ set +x

== merged 155
++ echo inner
+ x=inner
+ echo sub
sub
+++ echo deep
++ echo deep
+ y=deep
+ eval 'echo evald'
++ echo evald
evald
++ echo funsub
+ z=funsub
+ set +x
done

== status 0
