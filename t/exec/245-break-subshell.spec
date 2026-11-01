== stdout 118
subshell continues 1
loop 1
subshell continues 2
loop 2
inner loop broke 1
inner loop broke 2
x=
x=
status 3
status 3

== stderr 148
bash: line 3: break: only meaningful in a `for', `while', or `until' loop
bash: line 3: break: only meaningful in a `for', `while', or `until' loop

== merged 266
bash: line 3: break: only meaningful in a `for', `while', or `until' loop
subshell continues 1
loop 1
bash: line 3: break: only meaningful in a `for', `while', or `until' loop
subshell continues 2
loop 2
inner loop broke 1
inner loop broke 2
x=
x=
status 3
status 3

== status 0
