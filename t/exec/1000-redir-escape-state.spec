== stdout 35
traced
traced2
done 200
status 127

== stderr 304
+ cat
++ eval n=1+
+++ n=1+
bash: line 6: 1+: arithmetic syntax error: operand expected (error token is "+")
+ echo traced
+ cat
++ . /dev/stdin
+++ n=1+
/dev/stdin: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
+ echo traced2
+ set +x
bash: line 14: nope: parameter not set

== merged 339
+ cat
++ eval n=1+
+++ n=1+
bash: line 6: 1+: arithmetic syntax error: operand expected (error token is "+")
+ echo traced
traced
+ cat
++ . /dev/stdin
+++ n=1+
/dev/stdin: line 1: 1+: arithmetic syntax error: operand expected (error token is "+")
+ echo traced2
traced2
+ set +x
done 200
bash: line 14: nope: parameter not set
status 127

== status 1
