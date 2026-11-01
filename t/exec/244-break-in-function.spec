== stdout 84
f 1
f after 0
g after 0
loop 1
f 2
f after 0
g after 0
loop 2
h a
h done
h a
h done

== stderr 302
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
bash: line 3: continue: only meaningful in a `for', `while', or `until' loop
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
bash: line 3: continue: only meaningful in a `for', `while', or `until' loop

== merged 386
f 1
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
f after 0
bash: line 3: continue: only meaningful in a `for', `while', or `until' loop
g after 0
loop 1
f 2
bash: line 2: break: only meaningful in a `for', `while', or `until' loop
f after 0
bash: line 3: continue: only meaningful in a `for', `while', or `until' loop
g after 0
loop 2
h a
h done
h a
h done

== status 0
