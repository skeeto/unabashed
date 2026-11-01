== stdout 110
a1
a2
a3
b1
b2
b3
in x
in x
bash: line 5: break: only meaningful in a `for', `while', or `until' loop
st=0
c2

== stderr 222
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
bash: line 2: break: only meaningful in a `for', `while', or `until' loop

== merged 332
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
a1
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
a2
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
a3
b1
b2
b3
in x
in x
bash: line 5: break: only meaningful in a `for', `while', or `until' loop
st=0
c2

== status 0
